#ifndef LEARNQL_RANGES_PROXY_VECTOR_HPP
#define LEARNQL_RANGES_PROXY_VECTOR_HPP

#include <vector>
#include <memory>
#include <iterator>
#include <functional>
#include <stdexcept>

namespace learnql::ranges {

/**
 * @brief Lazy-loading proxy vector with batched data retrieval
 * @tparam T The element type
 * @tparam BatchSize Number of elements to load per batch (compile-time configurable)
 *
 * ProxyVector provides a std::vector-like interface but loads data lazily
 * in batches. This significantly reduces memory usage for large datasets
 * by only keeping the current batch in memory.
 *
 * Features:
 * - Transparent std::vector-like interface
 * - Lazy batch loading (only loads data when accessed)
 * - Memory-efficient (discards old batches after iteration)
 * - Forward-only iteration (no random access or backward iteration)
 * - STL-compatible iterators
 * - Compile-time configurable batch size
 *
 * Design:
 * - Uses a batch fetcher function to retrieve data incrementally
 * - Iterator maintains position and triggers batch loading
 * - Old batches are automatically discarded
 *
 * Example:
 * @code
 * // Create a fetcher that loads student records in batches
 * auto fetcher = []() -> std::vector<Student> {
 *     return load_next_batch_from_db();
 * };
 *
 * ProxyVector<Student, 10> students(fetcher);
 *
 * // Transparent usage - works like std::vector
 * for (const auto& student : students) {
 *     std::cout << student.name << std::endl;
 * }
 * @endcode
 */
template<typename T, std::size_t BatchSize = 10>
class ProxyVector {
private:
    /**
     * @brief Batch fetcher function type
     *
     * This function is called to retrieve the next batch of elements.
     * It should return a vector with up to BatchSize elements, or an
     * empty vector if no more data is available.
     */
    using BatchFetcher = std::function<std::vector<T>()>;

    BatchFetcher batch_fetcher_;        ///< Function to fetch next batch
    mutable std::vector<T> current_batch_; ///< Current batch in memory
    mutable std::size_t global_pos_;    ///< Global position across all batches
    mutable std::size_t batches_loaded_; ///< Number of batches loaded so far
    mutable bool exhausted_;            ///< True if all data has been loaded

public:
    // STL type aliases for compatibility
    using value_type = T;
    using size_type = std::size_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;

    /**
     * @brief Forward iterator for ProxyVector
     *
     * This iterator triggers lazy batch loading as it advances.
     * It automatically fetches the next batch when the current batch
     * is exhausted. Old batches are discarded to minimize memory usage.
     */
    class Iterator {
    private:
        ProxyVector* proxy_;           ///< Pointer to parent ProxyVector
        std::size_t global_position_;  ///< Global position across all batches
        std::size_t batch_index_;      ///< Index within current batch
        bool is_end_;                  ///< True if this is the end iterator

        friend class ProxyVector;

        /**
         * @brief Constructor for begin iterator
         */
        Iterator(ProxyVector* proxy, std::size_t global_pos, std::size_t batch_idx, bool is_end)
            : proxy_(proxy),
              global_position_(global_pos),
              batch_index_(batch_idx),
              is_end_(is_end) {

            // Load first batch if needed
            if (!is_end_ && proxy_->current_batch_.empty() && !proxy_->exhausted_) {
                proxy_->load_next_batch();
            }
        }

    public:
        // STL iterator type aliases
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        /**
         * @brief Dereferences the iterator
         * @return Reference to the current element
         */
        reference operator*() const {
            if (is_end_ || batch_index_ >= proxy_->current_batch_.size()) {
                throw std::out_of_range("ProxyVector::Iterator: dereferencing end iterator");
            }
            return proxy_->current_batch_[batch_index_];
        }

        /**
         * @brief Arrow operator for member access
         * @return Pointer to the current element
         */
        pointer operator->() const {
            if (is_end_ || batch_index_ >= proxy_->current_batch_.size()) {
                throw std::out_of_range("ProxyVector::Iterator: dereferencing end iterator");
            }
            return &proxy_->current_batch_[batch_index_];
        }

        /**
         * @brief Pre-increment operator
         * @return Reference to this iterator after increment
         *
         * Advances to the next element. If the current batch is exhausted,
         * automatically loads the next batch. Old batch data is discarded.
         */
        Iterator& operator++() {
            if (is_end_) {
                return *this;
            }

            ++global_position_;
            ++batch_index_;

            // Check if we've exhausted the current batch
            if (batch_index_ >= proxy_->current_batch_.size()) {
                if (proxy_->exhausted_) {
                    // No more data available
                    is_end_ = true;
                } else {
                    // Load next batch and reset batch index
                    proxy_->load_next_batch();
                    batch_index_ = 0;

                    // Check if new batch is empty (end of data)
                    if (proxy_->current_batch_.empty()) {
                        is_end_ = true;
                        proxy_->exhausted_ = true;
                    }
                }
            }

            return *this;
        }

        /**
         * @brief Post-increment operator
         * @return Copy of iterator before increment
         */
        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        /**
         * @brief Equality comparison
         */
        bool operator==(const Iterator& other) const {
            if (is_end_ && other.is_end_) {
                return true;
            }
            if (is_end_ != other.is_end_) {
                return false;
            }
            return proxy_ == other.proxy_ && global_position_ == other.global_position_;
        }

        /**
         * @brief Inequality comparison
         */
        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }
    };

    using iterator = Iterator;
    using const_iterator = Iterator; // Forward-only, so const is the same

    /**
     * @brief Constructs a ProxyVector with a batch fetcher
     * @param fetcher Function that returns the next batch of elements
     *
     * The fetcher should return up to BatchSize elements per call,
     * or an empty vector when no more data is available.
     */
    explicit ProxyVector(BatchFetcher fetcher)
        : batch_fetcher_(std::move(fetcher)),
          current_batch_(),
          global_pos_(0),
          batches_loaded_(0),
          exhausted_(false) {
        current_batch_.reserve(BatchSize);
    }

    /**
     * @brief Returns an iterator to the beginning
     * @return Iterator pointing to the first element
     */
    [[nodiscard]] iterator begin() {
        return Iterator(this, 0, 0, false);
    }

    /**
     * @brief Returns an iterator to the end
     * @return Iterator representing the end of the sequence
     */
    [[nodiscard]] iterator end() {
        return Iterator(this, 0, 0, true);
    }

    /**
     * @brief Returns a const iterator to the beginning
     */
    [[nodiscard]] const_iterator begin() const {
        return Iterator(const_cast<ProxyVector*>(this), 0, 0, false);
    }

    /**
     * @brief Returns a const iterator to the end
     */
    [[nodiscard]] const_iterator end() const {
        return Iterator(const_cast<ProxyVector*>(this), 0, 0, true);
    }

    /**
     * @brief Returns a const iterator to the beginning
     */
    [[nodiscard]] const_iterator cbegin() const {
        return begin();
    }

    /**
     * @brief Returns a const iterator to the end
     */
    [[nodiscard]] const_iterator cend() const {
        return end();
    }

    /**
     * @brief Checks if the proxy vector is empty
     * @return true if no data is available
     *
     * Note: This loads the first batch to determine if data exists.
     */
    [[nodiscard]] bool empty() const {
        if (current_batch_.empty() && !exhausted_) {
            load_next_batch();
        }
        return current_batch_.empty() && exhausted_;
    }

    /**
     * @brief Materializes all remaining data into a std::vector
     * @return Vector containing all elements
     *
     * Use this method when you need all data in memory at once.
     * This defeats the purpose of lazy loading, so use sparingly.
     */
    [[nodiscard]] std::vector<T> materialize() const {
        std::vector<T> result;

        // Start from current batch
        result.insert(result.end(), current_batch_.begin(), current_batch_.end());

        // Load and append remaining batches
        while (!exhausted_) {
            load_next_batch();
            result.insert(result.end(), current_batch_.begin(), current_batch_.end());
        }

        return result;
    }

private:
    /**
     * @brief Loads the next batch of data
     *
     * This method is called by the iterator when the current batch
     * is exhausted. The old batch is discarded to free memory.
     */
    void load_next_batch() const {
        if (exhausted_) {
            return;
        }

        // Fetch next batch
        std::vector<T> new_batch = batch_fetcher_();

        // Replace current batch (old batch is discarded)
        current_batch_ = std::move(new_batch);
        ++batches_loaded_;

        // If batch is smaller than expected or empty, we've reached the end
        if (current_batch_.size() < BatchSize || current_batch_.empty()) {
            exhausted_ = true;
        }
    }
};

} // namespace learnql::ranges

#endif // LEARNQL_RANGES_PROXY_VECTOR_HPP
