#ifndef LEARNQL_RANGES_QUERY_VIEW_HPP
#define LEARNQL_RANGES_QUERY_VIEW_HPP

#include <ranges>
#include <vector>
#include <memory>
#include <concepts>
#include "ProxyVector.hpp"

namespace learnql::ranges {

/**
 * @brief A custom range view that wraps query results
 * @tparam T The element type
 *
 * This view provides lazy access to query results and integrates
 * seamlessly with C++20 ranges library.
 *
 * Example:
 * @code
 * auto view = QueryView(students.all());
 * auto result = view
 *     | std::views::filter([](auto& s) { return s.age > 20; })
 *     | std::views::transform([](auto& s) { return s.name; });
 * @endcode
 */
template<typename T>
class QueryView : public std::ranges::view_interface<QueryView<T>> {
public:
    /**
     * @brief Iterator for the query view
     */
    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        Iterator() = default;

        explicit Iterator(typename std::vector<T>::iterator it)
            : it_(it) {}

        reference operator*() const {
            return *it_;
        }

        pointer operator->() const {
            return &(*it_);
        }

        Iterator& operator++() {
            ++it_;
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const Iterator& a, const Iterator& b) {
            return a.it_ == b.it_;
        }

        friend bool operator!=(const Iterator& a, const Iterator& b) {
            return !(a == b);
        }

    private:
        typename std::vector<T>::iterator it_;
    };

    /**
     * @brief Constructs a query view from a vector of results
     * @param data The query result data
     */
    explicit QueryView(std::vector<T> data)
        : data_(std::make_shared<std::vector<T>>(std::move(data))) {}

    /**
     * @brief Constructs a query view from a ProxyVector
     * @tparam BatchSize The batch size of the ProxyVector
     * @param proxy_data The proxy vector to materialize
     *
     * Note: This materializes the ProxyVector into memory. For large datasets,
     * prefer iterating over the ProxyVector directly.
     */
    template<std::size_t BatchSize>
    explicit QueryView(ProxyVector<T, BatchSize> proxy_data)
        : data_(std::make_shared<std::vector<T>>(proxy_data.materialize())) {}

    /**
     * @brief Gets the begin iterator
     */
    [[nodiscard]] Iterator begin() {
        return Iterator(data_->begin());
    }

    /**
     * @brief Gets the end iterator
     */
    [[nodiscard]] Iterator end() {
        return Iterator(data_->end());
    }

    /**
     * @brief Gets the begin iterator (const version)
     */
    [[nodiscard]] Iterator begin() const {
        return Iterator(const_cast<std::vector<T>&>(*data_).begin());
    }

    /**
     * @brief Gets the end iterator (const version)
     */
    [[nodiscard]] Iterator end() const {
        return Iterator(const_cast<std::vector<T>&>(*data_).end());
    }

    /**
     * @brief Checks if the view is empty
     */
    [[nodiscard]] bool empty() const noexcept {
        return data_->empty();
    }

    /**
     * @brief Gets the size of the view
     */
    [[nodiscard]] std::size_t size() const noexcept {
        return data_->size();
    }

    /**
     * @brief Materializes the view into a vector
     */
    [[nodiscard]] std::vector<T> to_vector() const {
        return *data_;
    }

private:
    std::shared_ptr<std::vector<T>> data_;
};

// Deduction guides
template<typename T>
QueryView(std::vector<T>) -> QueryView<T>;

template<typename T, std::size_t BatchSize>
QueryView(ProxyVector<T, BatchSize>) -> QueryView<T>;

} // namespace learnql::ranges

#endif // LEARNQL_RANGES_QUERY_VIEW_HPP
