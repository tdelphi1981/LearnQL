#ifndef LEARNQL_INDEX_BATCH_ITERATOR_HPP
#define LEARNQL_INDEX_BATCH_ITERATOR_HPP

#include <vector>
#include <stack>
#include <utility>
#include <cstddef>

namespace learnql::index {

/**
 * @brief Batch iterator for B+tree traversal
 * @tparam Key The key type
 * @tparam Value The value type (typically RecordId)
 * @tparam BatchSize The number of entries to fetch per batch
 *
 * This iterator performs efficient B+tree traversal by walking the leaf
 * linked list, yielding entries in batches. In B+Tree mode, it's much
 * faster than tree traversal since all data is in leaves.
 *
 * Features:
 * - B+Tree optimized: walks leaf linked list sequentially
 * - Stateful cursor-based traversal
 * - Configurable batch size at compile time
 * - Memory-efficient (only current batch in memory)
 * - In-order (sorted) traversal
 *
 * Example:
 * @code
 * BatchIterator<int, RecordId, 10> iter(index);
 * while (iter.has_more()) {
 *     auto batch = iter.next_batch();
 *     for (const auto& [key, value] : batch) {
 *         // Process each entry
 *     }
 * }
 * @endcode
 */
template<typename Key, typename Value, std::size_t BatchSize = 10>
class BatchIterator {
private:
    // B+Tree leaf cursor
    uint64_t current_leaf_id_;     ///< Current leaf page ID
    std::size_t current_key_index_; ///< Current index within leaf
    bool exhausted_;                ///< True if traversal is complete
    uint64_t root_page_id_;        ///< Root page ID of the B+tree

    // Function to load a node (provided by the index)
    // Extended to support B+Tree: also returns next_page_id for leaf linking
    std::function<void(uint64_t, std::vector<Key>&, std::vector<Value>&,
                       std::vector<uint64_t>&, bool&, uint64_t&)> load_node_fn_;

public:
    /**
     * @brief Constructs a batch iterator for B+Tree
     * @param root_page_id Root page ID of the B+tree
     * @param load_fn Function to load node data given a page ID
     *
     * The load_fn should populate the provided references with node data:
     * - keys: vector of keys in the node
     * - values: vector of values in the node (leaf nodes only in B+Tree)
     * - children: vector of child page IDs (internal nodes only)
     * - is_leaf: whether the node is a leaf
     * - next_page_id: next leaf page ID (0 if none, for B+Tree linking)
     *
     * The iterator starts at the leftmost (first) leaf for efficient
     * B+Tree traversal via leaf linked list.
     */
    template<typename LoadFn>
    BatchIterator(uint64_t root_page_id, LoadFn&& load_fn)
        : current_leaf_id_(0),
          current_key_index_(0),
          exhausted_(root_page_id == 0),
          root_page_id_(root_page_id),
          load_node_fn_(std::forward<LoadFn>(load_fn)) {

        if (!exhausted_) {
            // Find the leftmost leaf to start iteration
            current_leaf_id_ = find_leftmost_leaf(root_page_id_);
            if (current_leaf_id_ == 0) {
                exhausted_ = true;
            }
        }
    }

    /**
     * @brief Checks if there are more entries to iterate
     * @return true if more entries exist
     */
    [[nodiscard]] bool has_more() const noexcept {
        return !exhausted_ && current_leaf_id_ != 0;
    }

    /**
     * @brief Fetches the next batch of entries (B+Tree optimized)
     * @return Vector of key-value pairs (up to BatchSize entries)
     *
     * This method walks the leaf linked list sequentially, collecting
     * up to BatchSize entries before returning. Much faster than tree
     * traversal since all data is in leaves.
     */
    [[nodiscard]] std::vector<std::pair<Key, Value>> next_batch() {
        std::vector<std::pair<Key, Value>> batch;
        batch.reserve(BatchSize);

        while (has_more() && batch.size() < BatchSize) {
            // Load current leaf
            std::vector<Key> keys;
            std::vector<Value> values;
            std::vector<uint64_t> children;  // Unused for leaves
            bool is_leaf = true;
            uint64_t next_page_id = 0;

            load_node_fn_(current_leaf_id_, keys, values, children, is_leaf, next_page_id);

            // Collect entries from current leaf starting at current_key_index_
            for (std::size_t i = current_key_index_; i < keys.size() && batch.size() < BatchSize; ++i) {
                batch.emplace_back(keys[i], values[i]);
                ++current_key_index_;
            }

            // If we've exhausted this leaf, move to the next one
            if (current_key_index_ >= keys.size()) {
                current_leaf_id_ = next_page_id;
                current_key_index_ = 0;

                if (current_leaf_id_ == 0) {
                    exhausted_ = true;
                }
            }
        }

        return batch;
    }

private:
    /**
     * @brief Finds the leftmost (first) leaf node in the B+Tree
     */
    uint64_t find_leftmost_leaf(uint64_t node_id) const {
        if (node_id == 0) {
            return 0;
        }

        std::vector<Key> keys;
        std::vector<Value> values;
        std::vector<uint64_t> children;
        bool is_leaf = true;
        uint64_t next_page_id = 0;

        load_node_fn_(node_id, keys, values, children, is_leaf, next_page_id);

        // If this is a leaf, we found it
        if (is_leaf) {
            return node_id;
        }

        // Internal node: follow the leftmost child
        if (!children.empty()) {
            return find_leftmost_leaf(children[0]);
        }

        return 0;
    }

public:

    /**
     * @brief Resets the iterator to the beginning
     *
     * Allows re-iteration from the start. Finds the leftmost leaf
     * and resets the cursor to the beginning.
     */
    void reset() {
        current_key_index_ = 0;
        exhausted_ = (root_page_id_ == 0);

        if (!exhausted_) {
            current_leaf_id_ = find_leftmost_leaf(root_page_id_);
            if (current_leaf_id_ == 0) {
                exhausted_ = true;
            }
        }
    }
};

} // namespace learnql::index

#endif // LEARNQL_INDEX_BATCH_ITERATOR_HPP
