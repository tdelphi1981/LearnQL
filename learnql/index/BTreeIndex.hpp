#ifndef LEARNQL_INDEX_BTREE_INDEX_HPP
#define LEARNQL_INDEX_BTREE_INDEX_HPP

#include "../core/RecordId.hpp"
#include <vector>
#include <memory>
#include <algorithm>
#include <optional>

namespace learnql::index {

/**
 * @brief DEPRECATED: In-memory B-tree index for efficient key lookups
 * @tparam Key The key type (must be comparable)
 * @tparam Value The value type (typically RecordId)
 *
 * @deprecated This in-memory implementation has been replaced by PersistentBTreeIndex.
 *             Use PersistentBTreeIndex for production code as it persists data to disk.
 *             This class is kept for educational/reference purposes only.
 *
 * This is a simplified B-tree implementation for educational purposes.
 * This implementation does NOT persist to disk - all data is lost on restart.
 *
 * Features:
 * - O(log n) search, insert, delete
 * - Range queries
 * - Ordered iteration
 *
 * Example:
 * @code
 * // DEPRECATED - Use PersistentBTreeIndex instead!
 * BTreeIndex<int, RecordId> index;
 * index.insert(42, RecordId{1, 0});
 * auto rid = index.find(42);
 * @endcode
 */
template<typename Key, typename Value = core::RecordId>
requires std::totally_ordered<Key>
class [[deprecated("Use PersistentBTreeIndex instead - this in-memory version does not persist to disk")]] BTreeIndex {
private:
    static constexpr std::size_t ORDER = 4; // B-tree order (max children per node)
    static constexpr std::size_t MIN_KEYS = ORDER / 2 - 1;
    static constexpr std::size_t MAX_KEYS = ORDER - 1;

    /**
     * @brief Node in the B-tree
     */
    struct Node {
        std::vector<Key> keys;
        std::vector<Value> values;
        std::vector<std::unique_ptr<Node>> children;
        bool is_leaf = true;

        Node() {
            keys.reserve(MAX_KEYS);
            values.reserve(MAX_KEYS);
            children.reserve(ORDER);
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
     * @brief Constructs an empty B-tree index
     */
    BTreeIndex() : root_(std::make_unique<Node>()), size_(0) {}

    /**
     * @brief Inserts a key-value pair
     * @param key The key
     * @param value The value
     * @return true if inserted, false if key already exists
     */
    bool insert(const Key& key, const Value& value) {
        // If root is full, split it
        if (root_->is_full()) {
            auto new_root = std::make_unique<Node>();
            new_root->is_leaf = false;
            new_root->children.push_back(std::move(root_));
            split_child(new_root.get(), 0);
            root_ = std::move(new_root);
        }

        bool inserted = insert_non_full(root_.get(), key, value);
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
        return search(root_.get(), key);
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
        if (!root_) {
            return false;
        }

        bool removed = remove_from_node(root_.get(), key);

        // If root is empty and has children, make the first child the new root
        if (root_->keys.empty() && !root_->is_leaf && !root_->children.empty()) {
            root_ = std::move(root_->children[0]);
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
        range_query_helper(root_.get(), min_key, max_key, results);
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
     * @brief Clears all entries from the index
     */
    void clear() {
        root_ = std::make_unique<Node>();
        size_ = 0;
    }

    /**
     * @brief Gets all key-value pairs in sorted order
     */
    [[nodiscard]] std::vector<std::pair<Key, Value>> get_all() const {
        std::vector<std::pair<Key, Value>> results;
        results.reserve(size_);
        traverse(root_.get(), results);
        return results;
    }

private:
    /**
     * @brief Searches for a key in a subtree
     */
    std::optional<Value> search(Node* node, const Key& key) const {
        if (!node) {
            return std::nullopt;
        }

        std::size_t i = 0;
        while (i < node->keys.size() && key > node->keys[i]) {
            ++i;
        }

        if (i < node->keys.size() && key == node->keys[i]) {
            return node->values[i];
        }

        if (node->is_leaf) {
            return std::nullopt;
        }

        return search(node->children[i].get(), key);
    }

    /**
     * @brief Inserts into a non-full node
     */
    bool insert_non_full(Node* node, const Key& key, const Value& value) {
        std::size_t i = node->find_insert_pos(key);

        // Check if key already exists
        if (i < node->keys.size() && node->keys[i] == key) {
            return false; // Duplicate key
        }

        if (node->is_leaf) {
            node->keys.insert(node->keys.begin() + i, key);
            node->values.insert(node->values.begin() + i, value);
            return true;
        } else {
            if (node->children[i]->is_full()) {
                split_child(node, i);
                if (key > node->keys[i]) {
                    ++i;
                }
            }
            return insert_non_full(node->children[i].get(), key, value);
        }
    }

    /**
     * @brief Splits a full child node
     */
    void split_child(Node* parent, std::size_t index) {
        Node* full_child = parent->children[index].get();
        auto new_node = std::make_unique<Node>();
        new_node->is_leaf = full_child->is_leaf;

        std::size_t mid = MAX_KEYS / 2;

        // Move second half of keys and values to new node
        new_node->keys.assign(
            full_child->keys.begin() + mid + 1,
            full_child->keys.end()
        );
        new_node->values.assign(
            full_child->values.begin() + mid + 1,
            full_child->values.end()
        );

        // If not a leaf, move children too
        if (!full_child->is_leaf) {
            new_node->children.assign(
                std::make_move_iterator(full_child->children.begin() + mid + 1),
                std::make_move_iterator(full_child->children.end())
            );
            full_child->children.resize(mid + 1);
        }

        // Promote middle key to parent
        parent->keys.insert(parent->keys.begin() + index, full_child->keys[mid]);
        parent->values.insert(parent->values.begin() + index, full_child->values[mid]);
        parent->children.insert(parent->children.begin() + index + 1, std::move(new_node));

        // Shrink original child
        full_child->keys.resize(mid);
        full_child->values.resize(mid);
    }

    /**
     * @brief Removes a key from a node
     */
    bool remove_from_node(Node* node, const Key& key) {
        std::size_t i = node->find_insert_pos(key);

        if (i < node->keys.size() && node->keys[i] == key) {
            if (node->is_leaf) {
                node->keys.erase(node->keys.begin() + i);
                node->values.erase(node->values.begin() + i);
                return true;
            } else {
                // Internal node removal (simplified - not fully implemented)
                return false;
            }
        } else if (!node->is_leaf) {
            return remove_from_node(node->children[i].get(), key);
        }

        return false;
    }

    /**
     * @brief Helper for range queries
     */
    void range_query_helper(Node* node, const Key& min_key, const Key& max_key,
                           std::vector<Value>& results) const {
        if (!node) {
            return;
        }

        std::size_t i = 0;
        while (i < node->keys.size()) {
            if (!node->is_leaf) {
                range_query_helper(node->children[i].get(), min_key, max_key, results);
            }

            if (node->keys[i] >= min_key && node->keys[i] <= max_key) {
                results.push_back(node->values[i]);
            }

            ++i;
        }

        if (!node->is_leaf && i < node->children.size()) {
            range_query_helper(node->children[i].get(), min_key, max_key, results);
        }
    }

    /**
     * @brief Traverses the tree in order
     */
    void traverse(Node* node, std::vector<std::pair<Key, Value>>& results) const {
        if (!node) {
            return;
        }

        std::size_t i = 0;
        while (i < node->keys.size()) {
            if (!node->is_leaf) {
                traverse(node->children[i].get(), results);
            }
            results.emplace_back(node->keys[i], node->values[i]);
            ++i;
        }

        if (!node->is_leaf && i < node->children.size()) {
            traverse(node->children[i].get(), results);
        }
    }

private:
    std::unique_ptr<Node> root_;
    std::size_t size_;
};

} // namespace learnql::index

#endif // LEARNQL_INDEX_BTREE_INDEX_HPP
