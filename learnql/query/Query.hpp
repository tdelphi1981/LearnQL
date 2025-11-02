#ifndef LEARNQL_QUERY_QUERY_HPP
#define LEARNQL_QUERY_QUERY_HPP

#include "Field.hpp"
#include "../core/Table.hpp"
#include <vector>
#include <functional>

namespace learnql::query {

/**
 * @brief Query builder for tables using expression templates
 * @tparam T Type of objects in the table
 * @tparam BatchSize Number of records to load per batch (default: 10)
 *
 * Provides a fluent interface for building SQL-like queries with
 * memory-efficient batched execution.
 *
 * Example:
 * @code
 * Table<Student> students;
 * Field<Student, int> age{"age", &Student::getAge};
 *
 * auto results = Query{students}.where(age > 18).execute();
 * @endcode
 */
template<typename T, std::size_t BatchSize = 10>
requires concepts::Queryable<T, serialization::BinaryWriter, serialization::BinaryReader>
class Query {
public:
    using table_type = core::Table<T, BatchSize>;
    using value_type = T;

    /**
     * @brief Constructs a query for a table
     * @param table Reference to the table
     */
    explicit Query(table_type& table)
        : table_(table), predicate_{} {}

    /**
     * @brief Adds a WHERE clause using an expression
     * @tparam ExprType Expression type
     * @param expr Expression to filter by
     * @return Reference to this query (for chaining)
     *
     * Example:
     * @code
     * query.where((age > 18) && (name == "Alice"));
     * @endcode
     */
    template<typename ExprType>
    requires Expression<ExprType>
    Query& where(const Expr<ExprType>& expr) {
        // Convert expression to a predicate function
        predicate_ = [expr_copy = expr.derived()](const T& obj) -> bool {
            return expr_copy.evaluate(obj);
        };
        return *this;
    }

    /**
     * @brief Executes the query and returns all matching records
     * @return ProxyVector of matching records (loaded in batches)
     */
    [[nodiscard]] ranges::ProxyVector<T, BatchSize> execute() const {
        if (predicate_) {
            return table_.find_if(predicate_);
        } else {
            return table_.get_all();
        }
    }

    /**
     * @brief Executes query and returns the first matching record
     * @return Unique pointer to the first match, or nullptr
     */
    [[nodiscard]] std::unique_ptr<T> execute_single() const {
        auto results = execute();

        // Use iterator to get first element without materializing all data
        auto it = results.begin();
        if (it != results.end()) {
            return std::make_unique<T>(*it);
        }

        return nullptr;
    }

    /**
     * @brief Counts matching records
     * @return Number of matching records
     */
    [[nodiscard]] std::size_t count() const {
        if (predicate_) {
            std::size_t cnt = 0;
            for (const auto& record : table_) {
                if (predicate_(record)) {
                    ++cnt;
                }
            }
            return cnt;
        } else {
            return table_.size();
        }
    }

    /**
     * @brief Checks if any records match the query
     * @return true if at least one record matches
     */
    [[nodiscard]] bool any() const {
        if (predicate_) {
            for (const auto& record : table_) {
                if (predicate_(record)) {
                    return true;
                }
            }
            return false;
        } else {
            return !table_.empty();
        }
    }

    /**
     * @brief Checks if all records match the query
     * @return true if all records match (or table is empty)
     */
    [[nodiscard]] bool all() const {
        if (predicate_) {
            for (const auto& record : table_) {
                if (!predicate_(record)) {
                    return false;
                }
            }
            return true;
        } else {
            return true;
        }
    }

    /**
     * @brief Implicit conversion to vector (executes and materializes the query)
     *
     * Note: This materializes all results into memory. For large result sets,
     * prefer using the execute() method and iterating over the ProxyVector.
     */
    operator std::vector<T>() const {
        return execute().materialize();
    }

    /**
     * @brief Provides begin() for range-based for loops
     * @return Iterator to matching elements
     */
    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        Iterator(typename table_type::Iterator it,
                typename table_type::Iterator end,
                std::function<bool(const T&)> pred)
            : it_(it), end_(end), predicate_(std::move(pred)) {
            advance_to_next_match();
        }

        reference operator*() const {
            return *it_;
        }

        pointer operator->() const {
            return &(*it_);
        }

        Iterator& operator++() {
            ++it_;
            advance_to_next_match();
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const Iterator& other) const {
            return it_ == other.it_;
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

    private:
        void advance_to_next_match() {
            while (it_ != end_ && predicate_ && !predicate_(*it_)) {
                ++it_;
            }
        }

        typename table_type::Iterator it_;
        typename table_type::Iterator end_;
        std::function<bool(const T&)> predicate_;
    };

    [[nodiscard]] Iterator begin() const {
        return Iterator{table_.begin(), table_.end(), predicate_};
    }

    [[nodiscard]] Iterator end() const {
        return Iterator{table_.end(), table_.end(), predicate_};
    }

private:
    table_type& table_;
    std::function<bool(const T&)> predicate_;
};

} // namespace learnql::query

#endif // LEARNQL_QUERY_QUERY_HPP
