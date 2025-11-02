#ifndef LEARNQL_INDEX_PERSISTENT_BTREE_INDEX_HPP
#define LEARNQL_INDEX_PERSISTENT_BTREE_INDEX_HPP

#include "../core/RecordId.hpp"
#include "../storage/StorageEngine.hpp"
#include "../storage/Page.hpp"
#include "../serialization/BinaryWriter.hpp"
#include "../serialization/BinaryReader.hpp"
#include "BatchIterator.hpp"
#include <vector>
#include <memory>
#include <algorithm>
#include <optional>
#include <unordered_map>
#include <stdexcept>

namespace learnql::index {

/**
 * @brief Persistent B+tree index that stores nodes to disk
 * @tparam Key The key type (must be comparable and serializable)
 * @tparam Value The value type (typically RecordId)
 *
 * This is a disk-based B+tree implementation that persists all nodes to INDEX pages.
 * Unlike the in-memory BTreeIndex, this version survives database restarts.
 *
 * B+Tree characteristics:
 * - All data (values) stored in leaf nodes only
 * - Internal nodes contain only keys for routing
 * - Leaf nodes linked in sequential order for fast range scans
 * - Higher fanout than B-Tree (internal nodes smaller)
 * - Consistent O(log n) search (always to leaf level)
 *
 * Features:
 * - O(log n) search, insert, delete (with disk I/O)
 * - O(k) range queries via leaf linking (k = result size)
 * - Persistent storage using INDEX pages
 * - Node caching for performance
 * - Lazy loading of nodes
 * - Page-based serialization
 *
 * Architecture:
 * - Each B+tree node is stored in a separate INDEX page
 * - Nodes reference children by page ID instead of pointers
 * - Leaf nodes linked via next/prev page IDs
 * - Root page ID is stored in metadata for recovery
 * - LRU-like node cache reduces disk I/O
 *
 * Example:
 * @code
 * auto storage = std::make_shared<StorageEngine>("data.db");
 * PersistentBTreeIndex<int, RecordId> index(storage);
 * index.insert(42, RecordId{1, 0});
 * auto rid = index.find(42);
 * index.flush(); // Persist all changes
 * @endcode
 */
template<typename Key, typename Value = core::RecordId>
requires std::totally_ordered<Key>
class PersistentBTreeIndex {
private:
    static constexpr std::size_t ORDER = 4; // B-tree order (max children per node)
    static constexpr std::size_t MIN_KEYS = ORDER / 2 - 1;
    static constexpr std::size_t MAX_KEYS = ORDER - 1;
    static constexpr std::size_t CACHE_SIZE = 32; // Number of nodes to cache

    /**
     * @brief Serializable B+tree node structure
     *
     * Unlike the in-memory version, this uses page IDs instead of pointers.
     * The node can be serialized to/from a Page.
     *
     * B+Tree structure:
     * - Leaf nodes: contain keys, values, and links to next/prev leaves
     * - Internal nodes: contain keys (for routing) and child pointers
     *   (values vector exists but is unused in internal nodes)
     */
    struct Node {
        uint64_t page_id;                    ///< Page ID of this node
        std::vector<Key> keys;               ///< Keys in sorted order
        std::vector<Value> values;           ///< Values (used only in leaf nodes)
        std::vector<uint64_t> children_ids;  ///< Page IDs of children (internal nodes only)
        bool is_leaf;                        ///< True if this is a leaf node
        uint64_t next_page_id;               ///< Next leaf page (0 if none, leaf nodes only)
        uint64_t prev_page_id;               ///< Previous leaf page (0 if none, leaf nodes only)

        Node() : page_id(0), is_leaf(true), next_page_id(0), prev_page_id(0) {
            keys.reserve(MAX_KEYS);
            values.reserve(MAX_KEYS);
            children_ids.reserve(ORDER);
        }

        explicit Node(uint64_t pid) : page_id(pid), is_leaf(true), next_page_id(0), prev_page_id(0) {
            keys.reserve(MAX_KEYS);
            values.reserve(MAX_KEYS);
            children_ids.reserve(ORDER);
        }

        /**
         * @brief Serializes the node to a BinaryWriter
         */
        void serialize(serialization::BinaryWriter& writer) const {
            writer.write(page_id);
            writer.write(is_leaf);
            writer.write(next_page_id);
            writer.write(prev_page_id);

            // Serialize keys
            writer.write(static_cast<uint32_t>(keys.size()));
            for (const auto& key : keys) {
                if constexpr (std::is_arithmetic_v<Key>) {
                    writer.write(key);
                } else if constexpr (requires { std::string(key); }) {
                    writer.write(std::string(key));
                } else {
                    // For custom types with serialize method
                    writer.write(key);
                }
            }

            // Serialize values (only meaningful for leaf nodes in B+Tree)
            writer.write(static_cast<uint32_t>(values.size()));
            for (const auto& value : values) {
                if constexpr (std::is_same_v<Value, core::RecordId>) {
                    writer.write(value.page_id);
                    writer.write(value.slot);
                } else if constexpr (std::is_arithmetic_v<Value>) {
                    writer.write(value);
                } else {
                    writer.write(value);
                }
            }

            // Serialize children page IDs (only for internal nodes)
            writer.write(static_cast<uint32_t>(children_ids.size()));
            for (uint64_t child_id : children_ids) {
                writer.write(child_id);
            }
        }

        /**
         * @brief Deserializes the node from a BinaryReader
         */
        void deserialize(serialization::BinaryReader& reader) {
            page_id = reader.read<uint64_t>();
            is_leaf = reader.read<bool>();
            next_page_id = reader.read<uint64_t>();
            prev_page_id = reader.read<uint64_t>();

            // Deserialize keys
            uint32_t keys_count = reader.read<uint32_t>();
            keys.clear();
            keys.reserve(keys_count);
            for (uint32_t i = 0; i < keys_count; ++i) {
                if constexpr (std::is_arithmetic_v<Key>) {
                    keys.push_back(reader.read<Key>());
                } else if constexpr (requires { std::string(Key{}); }) {
                    keys.push_back(Key(reader.read_string()));
                } else {
                    Key key;
                    key.deserialize(reader);
                    keys.push_back(std::move(key));
                }
            }

            // Deserialize values (only meaningful for leaf nodes in B+Tree)
            uint32_t values_count = reader.read<uint32_t>();
            values.clear();
            values.reserve(values_count);
            for (uint32_t i = 0; i < values_count; ++i) {
                if constexpr (std::is_same_v<Value, core::RecordId>) {
                    uint64_t page = reader.read<uint64_t>();
                    uint32_t slot = reader.read<uint32_t>();
                    values.push_back(core::RecordId{page, slot});
                } else if constexpr (std::is_arithmetic_v<Value>) {
                    values.push_back(reader.read<Value>());
                } else {
                    Value val;
                    val.deserialize(reader);
                    values.push_back(std::move(val));
                }
            }

            // Deserialize children page IDs (only for internal nodes)
            uint32_t children_count = reader.read<uint32_t>();
            children_ids.clear();
            children_ids.reserve(children_count);
            for (uint32_t i = 0; i < children_count; ++i) {
                children_ids.push_back(reader.read<uint64_t>());
            }
        }

        /**
         * @brief Finds the index where a key should be inserted
         */
        std::size_t find_insert_pos(const Key& key) const {
            return std::lower_bound(keys.begin(), keys.end(), key) - keys.begin();
        }

        /**
         * @brief Checks if node is full
         */
        [[nodiscard]] bool is_full() const {
            return keys.size() >= MAX_KEYS;
        }

        /**
         * @brief Checks if node has minimum keys
         */
        [[nodiscard]] bool has_min_keys() const {
            return keys.size() >= MIN_KEYS;
        }
    };

public:
    /**
     * @brief Constructs a persistent B-tree index
     * @param storage Shared pointer to the storage engine
     * @param root_page_id Optional root page ID (for loading existing index)
     */
    explicit PersistentBTreeIndex(std::shared_ptr<storage::StorageEngine> storage,
                                   uint64_t root_page_id = 0)
        : storage_(std::move(storage)),
          root_page_id_(root_page_id),
          size_(0),
          node_cache_(),
          dirty_nodes_() {

        if (root_page_id_ == 0) {
            // Create a new root node
            root_page_id_ = allocate_node(true);
        } else {
            // Load existing index - count entries by traversing
            size_ = count_entries(root_page_id_);
        }
    }

    /**
     * @brief Destructor - flushes all dirty nodes to disk
     */
    ~PersistentBTreeIndex() {
        try {
            flush();
        } catch (...) {
            // Suppress exceptions in destructor
        }
    }

    /**
     * @brief Inserts a key-value pair
     * @param key The key
     * @param value The value
     * @return true if inserted, false if key already exists
     */
    bool insert(const Key& key, const Value& value) {
        Node root = load_node(root_page_id_);

        // If root is full, split it
        if (root.is_full()) {
            uint64_t new_root_id = allocate_node(false);
            Node new_root = load_node(new_root_id);
            new_root.children_ids.push_back(root_page_id_);
            split_child(new_root, 0);
            save_node(new_root);
            root_page_id_ = new_root_id;
        }

        root = load_node(root_page_id_);
        bool inserted = insert_non_full(root, key, value);
        if (inserted) {
            ++size_;
        }
        return inserted;
    }

    /**
     * @brief Finds a value by key
     * @param key The key to search for
     * @return Optional containing the value if found
     */
    [[nodiscard]] std::optional<Value> find(const Key& key) const {
        return search(root_page_id_, key);
    }

    /**
     * @brief Checks if a key exists
     * @param key The key to check
     * @return true if key exists
     */
    [[nodiscard]] bool contains(const Key& key) const {
        return find(key).has_value();
    }

    /**
     * @brief Removes a key-value pair
     * @param key The key to remove
     * @return true if removed, false if not found
     */
    bool remove(const Key& key) {
        Node root = load_node(root_page_id_);
        bool removed = remove_from_node(root, key);

        // If root is empty and has children, make the first child the new root
        if (root.keys.empty() && !root.is_leaf && !root.children_ids.empty()) {
            uint64_t old_root_id = root_page_id_;
            root_page_id_ = root.children_ids[0];
            deallocate_node(old_root_id);
        } else {
            save_node(root);
        }

        if (removed) {
            --size_;
        }

        return removed;
    }

    /**
     * @brief Finds all values in a range [min_key, max_key]
     * @param min_key Minimum key (inclusive)
     * @param max_key Maximum key (inclusive)
     * @return Vector of values in range
     */
    [[nodiscard]] std::vector<Value> range_query(const Key& min_key, const Key& max_key) const {
        std::vector<Value> results;
        range_query_helper(root_page_id_, min_key, max_key, results);
        return results;
    }

    /**
     * @brief Gets the number of entries in the index
     */
    [[nodiscard]] std::size_t size() const noexcept {
        return size_;
    }

    /**
     * @brief Checks if the index is empty
     */
    [[nodiscard]] bool empty() const noexcept {
        return size_ == 0;
    }

    /**
     * @brief Gets the root page ID (for persistence)
     * @return Root page ID
     */
    [[nodiscard]] uint64_t get_root_page_id() const noexcept {
        return root_page_id_;
    }

    /**
     * @brief Gets all key-value pairs in sorted order
     * @return Vector of all key-value pairs
     */
    [[nodiscard]] std::vector<std::pair<Key, Value>> get_all() const {
        std::vector<std::pair<Key, Value>> results;
        results.reserve(size_);
        traverse(root_page_id_, results);
        return results;
    }

    /**
     * @brief Creates a batch iterator for lazy traversal (B+Tree optimized)
     * @tparam BatchSize Number of entries to fetch per batch
     * @return BatchIterator configured for this index
     *
     * This method creates a stateful iterator that traverses the B+tree
     * by walking the leaf linked list. This is much more efficient than
     * tree traversal and loads only BatchSize entries at a time.
     *
     * Example:
     * @code
     * auto iter = index.create_batch_iterator<10>();
     * while (iter.has_more()) {
     *     auto batch = iter.next_batch();
     *     // Process batch...
     * }
     * @endcode
     */
    template<std::size_t BatchSize = 10>
    [[nodiscard]] BatchIterator<Key, Value, BatchSize> create_batch_iterator() const {
        // Create a lambda that loads node data given a page ID
        // B+Tree version: also returns next_page_id for leaf linking
        auto load_fn = [this](uint64_t page_id,
                             std::vector<Key>& keys,
                             std::vector<Value>& values,
                             std::vector<uint64_t>& children,
                             bool& is_leaf,
                             uint64_t& next_page_id) {
            Node node = this->load_node(page_id);
            keys = node.keys;
            values = node.values;
            children = node.children_ids;
            is_leaf = node.is_leaf;
            next_page_id = node.next_page_id;
        };

        return BatchIterator<Key, Value, BatchSize>(root_page_id_, load_fn);
    }

    /**
     * @brief Flushes all dirty nodes to disk
     * @details Must be called to persist changes
     */
    void flush() {
        for (uint64_t page_id : dirty_nodes_) {
            auto it = node_cache_.find(page_id);
            if (it != node_cache_.end()) {
                write_node_to_page(it->second);
            }
        }
        dirty_nodes_.clear();
        storage_->flush_all();
    }

    /**
     * @brief Clears the node cache and flushes to disk
     */
    void clear_cache() {
        flush();
        node_cache_.clear();
    }

private:
    /**
     * @brief Allocates a new node page
     * @param is_leaf Whether the node is a leaf
     * @return Page ID of the allocated node
     */
    uint64_t allocate_node(bool is_leaf) {
        uint64_t page_id = storage_->allocate_page(storage::PageType::INDEX);
        Node node(page_id);
        node.is_leaf = is_leaf;
        save_node(node);
        return page_id;
    }

    /**
     * @brief Deallocates a node page
     * @param page_id Page ID to deallocate
     */
    void deallocate_node(uint64_t page_id) {
        node_cache_.erase(page_id);
        dirty_nodes_.erase(page_id);
        storage_->deallocate_page(page_id);
    }

    /**
     * @brief Loads a node from disk (with caching)
     * @param page_id Page ID of the node
     * @return The loaded node
     */
    Node load_node(uint64_t page_id) const {
        // Check cache first
        auto it = node_cache_.find(page_id);
        if (it != node_cache_.end()) {
            return it->second;
        }

        // Load from disk
        storage::Page page = storage_->read_page(page_id);
        Node node;

        // Deserialize the node from the page
        auto data_span = page.data();
        serialization::BinaryReader reader(data_span);
        node.deserialize(reader);

        // Add to cache (evict if necessary)
        if (node_cache_.size() >= CACHE_SIZE) {
            evict_node();
        }
        node_cache_[page_id] = node;

        return node;
    }

    /**
     * @brief Saves a node to cache and marks it dirty
     * @param node Node to save
     */
    void save_node(const Node& node) {
        node_cache_[node.page_id] = node;
        dirty_nodes_.insert(node.page_id);
    }

    /**
     * @brief Writes a node to its page
     * @param node Node to write
     */
    void write_node_to_page(const Node& node) const {
        storage::Page page(node.page_id, storage::PageType::INDEX);

        // Serialize the node
        serialization::BinaryWriter writer;
        node.serialize(writer);

        auto buffer = writer.get_buffer();
        if (buffer.size() > storage::Page::DATA_SIZE) {
            throw std::runtime_error("Node too large to fit in a single page");
        }

        // Write to page data
        page.write_data(0, buffer.data(), buffer.size());

        // Write the page to storage
        storage_->write_page(node.page_id, page);
    }

    /**
     * @brief Evicts a node from the cache (LRU-like)
     */
    void evict_node() const {
        if (node_cache_.empty()) {
            return;
        }

        // Simple eviction: remove first non-dirty node
        for (auto it = node_cache_.begin(); it != node_cache_.end(); ++it) {
            if (dirty_nodes_.find(it->first) == dirty_nodes_.end()) {
                node_cache_.erase(it);
                return;
            }
        }

        // If all nodes are dirty, flush and evict the first one
        uint64_t page_id = node_cache_.begin()->first;
        write_node_to_page(node_cache_.begin()->second);
        dirty_nodes_.erase(page_id);
        node_cache_.erase(page_id);
    }

    /**
     * @brief Searches for a key starting from a node (B+Tree: always to leaf)
     *
     * In B+Tree, all values are stored in leaves, so we always traverse
     * to the leaf level even if we encounter the key in an internal node.
     */
    std::optional<Value> search(uint64_t node_id, const Key& key) const {
        if (node_id == 0) {
            return std::nullopt;
        }

        Node node = load_node(node_id);

        // If this is a leaf node, search for the key
        if (node.is_leaf) {
            std::size_t i = 0;
            while (i < node.keys.size() && key > node.keys[i]) {
                ++i;
            }

            if (i < node.keys.size() && key == node.keys[i]) {
                return node.values[i];
            }
            return std::nullopt;
        }

        // Internal node: find the correct child to descend to
        // In B+Tree, keys are separators: key >= keys[i] means go to child[i+1]
        std::size_t i = 0;
        while (i < node.keys.size() && key >= node.keys[i]) {
            ++i;
        }

        // In B+Tree, keys in internal nodes are just routing guides
        // Always continue to child, even if key matches
        return search(node.children_ids[i], key);
    }

    /**
     * @brief Inserts into a non-full node (B+Tree: values only in leaves)
     *
     * In B+Tree, data is only stored in leaf nodes. Internal nodes
     * contain routing keys copied from leaves.
     */
    bool insert_non_full(Node& node, const Key& key, const Value& value) {
        std::size_t i = node.find_insert_pos(key);

        if (node.is_leaf) {
            // Check if key already exists in leaf
            if (i < node.keys.size() && node.keys[i] == key) {
                return false; // Duplicate key
            }

            // Insert key-value pair into leaf node
            node.keys.insert(node.keys.begin() + i, key);
            node.values.insert(node.values.begin() + i, value);
            save_node(node);
            return true;
        } else {
            // Internal node: find the correct child to descend to
            Node child = load_node(node.children_ids[i]);
            if (child.is_full()) {
                split_child(node, i);
                // After split, re-evaluate which child to use
                // In B+Tree: separator key K means left has keys < K, right has keys >= K
                if (key >= node.keys[i]) {
                    ++i;
                }
                child = load_node(node.children_ids[i]);
            }
            return insert_non_full(child, key, value);
        }
    }

    /**
     * @brief Splits a full child node (B+Tree version)
     *
     * B+Tree split behavior:
     * - Leaf split: Copy middle key up to parent, keep it in leaf, update leaf links
     * - Internal split: Push middle key up to parent (traditional B-tree behavior)
     */
    void split_child(Node& parent, std::size_t index) {
        Node full_child = load_node(parent.children_ids[index]);
        uint64_t new_node_id = allocate_node(full_child.is_leaf);
        Node new_node = load_node(new_node_id);

        std::size_t mid = MAX_KEYS / 2;

        if (full_child.is_leaf) {
            // LEAF NODE SPLIT (B+Tree specific)

            // Move second half of keys and values to new node
            // Note: For B+Tree, we copy the middle key up but keep it in the right node
            new_node.keys.assign(
                full_child.keys.begin() + mid,
                full_child.keys.end()
            );
            new_node.values.assign(
                full_child.values.begin() + mid,
                full_child.values.end()
            );

            // Update leaf links to maintain the linked list
            new_node.next_page_id = full_child.next_page_id;
            new_node.prev_page_id = full_child.page_id;
            full_child.next_page_id = new_node_id;

            // If there's a next leaf, update its prev pointer
            if (new_node.next_page_id != 0) {
                Node next_leaf = load_node(new_node.next_page_id);
                next_leaf.prev_page_id = new_node_id;
                save_node(next_leaf);
            }

            // Copy the first key of new_node up to parent (it stays in the leaf too)
            parent.keys.insert(parent.keys.begin() + index, new_node.keys[0]);
            parent.children_ids.insert(parent.children_ids.begin() + index + 1, new_node_id);

            // Shrink original child to first half
            full_child.keys.resize(mid);
            full_child.values.resize(mid);

        } else {
            // INTERNAL NODE SPLIT (standard B-tree behavior)

            // Move second half of keys to new node (excluding middle key)
            new_node.keys.assign(
                full_child.keys.begin() + mid + 1,
                full_child.keys.end()
            );

            // Move second half of children to new node
            new_node.children_ids.assign(
                full_child.children_ids.begin() + mid + 1,
                full_child.children_ids.end()
            );

            // Push middle key up to parent (don't keep it in children)
            parent.keys.insert(parent.keys.begin() + index, full_child.keys[mid]);
            parent.children_ids.insert(parent.children_ids.begin() + index + 1, new_node_id);

            // Shrink original child (excluding middle key)
            full_child.keys.resize(mid);
            full_child.children_ids.resize(mid + 1);
        }

        // Save all modified nodes
        save_node(full_child);
        save_node(new_node);
        save_node(parent);
    }

    /**
     * @brief Removes a key from a node
     *
     * In a B+Tree, all actual key-value pairs are stored in leaf nodes.
     * Internal nodes only contain routing keys to guide traversal.
     * Therefore, we should only remove from leaf nodes.
     */
    bool remove_from_node(Node& node, const Key& key) {
        if (node.is_leaf) {
            // Leaf node: linear search for exact key match
            std::size_t i = 0;
            while (i < node.keys.size() && key > node.keys[i]) {
                ++i;
            }

            if (i < node.keys.size() && key == node.keys[i]) {
                node.keys.erase(node.keys.begin() + i);
                node.values.erase(node.values.begin() + i);
                save_node(node);
                return true;
            }
            return false;
        } else {
            // Internal node: find the correct child to descend to
            // In B+Tree, keys are separators: key >= keys[i] means go to child[i+1]
            std::size_t i = 0;
            while (i < node.keys.size() && key >= node.keys[i]) {
                ++i;
            }

            // Traverse to the appropriate child
            if (i < node.children_ids.size()) {
                Node child = load_node(node.children_ids[i]);
                bool removed = remove_from_node(child, key);
                save_node(node);
                return removed;
            }

            return false;
        }
    }

    /**
     * @brief Helper for range queries (B+Tree optimized)
     *
     * In B+Tree, we traverse to the starting leaf, then walk the leaf
     * linked list sequentially. This is much faster than tree traversal.
     */
    void range_query_helper(uint64_t node_id, const Key& min_key, const Key& max_key,
                           std::vector<Value>& results) const {
        if (node_id == 0) {
            return;
        }

        // Step 1: Find the starting leaf node
        uint64_t leaf_id = find_leaf_for_key(node_id, min_key);
        if (leaf_id == 0) {
            return;
        }

        // Step 2: Walk the leaf linked list, collecting values in range
        while (leaf_id != 0) {
            Node leaf = load_node(leaf_id);

            // Collect values from this leaf that are in range
            for (std::size_t i = 0; i < leaf.keys.size(); ++i) {
                if (leaf.keys[i] < min_key) {
                    continue; // Skip keys below minimum
                }
                if (leaf.keys[i] > max_key) {
                    return; // All done, keys are sorted
                }
                results.push_back(leaf.values[i]);
            }

            // Move to next leaf in the linked list
            leaf_id = leaf.next_page_id;
        }
    }

    /**
     * @brief Finds the leaf node that would contain the given key
     *
     * This helper traverses from a starting node to the leaf level,
     * following the appropriate child pointers.
     */
    uint64_t find_leaf_for_key(uint64_t node_id, const Key& key) const {
        if (node_id == 0) {
            return 0;
        }

        Node node = load_node(node_id);

        // If this is a leaf, we found it
        if (node.is_leaf) {
            return node_id;
        }

        // Internal node: find the correct child to descend to
        std::size_t i = 0;
        while (i < node.keys.size() && key >= node.keys[i]) {
            ++i;
        }

        return find_leaf_for_key(node.children_ids[i], key);
    }

    /**
     * @brief Traverses all nodes in sorted order (B+Tree optimized)
     *
     * In B+Tree, all data is in leaves, so we find the leftmost leaf
     * and walk the leaf linked list.
     */
    void traverse(uint64_t node_id, std::vector<std::pair<Key, Value>>& results) const {
        if (node_id == 0) {
            return;
        }

        // Find the leftmost leaf (contains smallest keys)
        uint64_t leaf_id = find_leftmost_leaf(node_id);
        if (leaf_id == 0) {
            return;
        }

        // Walk the leaf linked list, collecting all key-value pairs
        while (leaf_id != 0) {
            Node leaf = load_node(leaf_id);

            for (std::size_t i = 0; i < leaf.keys.size(); ++i) {
                results.emplace_back(leaf.keys[i], leaf.values[i]);
            }

            // Move to next leaf
            leaf_id = leaf.next_page_id;
        }
    }

    /**
     * @brief Finds the leftmost (first) leaf node
     */
    uint64_t find_leftmost_leaf(uint64_t node_id) const {
        if (node_id == 0) {
            return 0;
        }

        Node node = load_node(node_id);

        // If this is a leaf, we found it
        if (node.is_leaf) {
            return node_id;
        }

        // Internal node: follow the leftmost child
        return find_leftmost_leaf(node.children_ids[0]);
    }

    /**
     * @brief Counts the number of entries in the index (B+Tree optimized)
     *
     * In B+Tree, all data is in leaves, so we walk the leaf linked list
     * and count entries in each leaf.
     */
    std::size_t count_entries(uint64_t node_id) const {
        if (node_id == 0) {
            return 0;
        }

        // Find the leftmost leaf
        uint64_t leaf_id = find_leftmost_leaf(node_id);
        if (leaf_id == 0) {
            return 0;
        }

        // Walk the leaf linked list and count entries
        std::size_t count = 0;
        while (leaf_id != 0) {
            Node leaf = load_node(leaf_id);
            count += leaf.keys.size();
            leaf_id = leaf.next_page_id;
        }

        return count;
    }

private:
    std::shared_ptr<storage::StorageEngine> storage_;  ///< Storage engine
    uint64_t root_page_id_;                            ///< Root node page ID
    std::size_t size_;                                 ///< Number of entries
    mutable std::unordered_map<uint64_t, Node> node_cache_; ///< Node cache
    mutable std::unordered_set<uint64_t> dirty_nodes_;      ///< Dirty node tracking
};

} // namespace learnql::index

#endif // LEARNQL_INDEX_PERSISTENT_BTREE_INDEX_HPP
