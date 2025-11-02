#ifndef LEARNQL_CORE_READONLYTABLE_HPP
#define LEARNQL_CORE_READONLYTABLE_HPP

#include "Table.hpp"
#include "../concepts/Queryable.hpp"
#include "../ranges/ProxyVector.hpp"
#include "../ranges/QueryView.hpp"
#include <memory>
#include <optional>

// Forward declaration
namespace learnql::catalog {
    class SystemCatalog;
}

namespace learnql::core {

/**
 * @brief Read-only wrapper around Table<T> that prevents mutations
 * @tparam T Type of objects stored (must satisfy Queryable concept)
 * @tparam BatchSize Number of records to load per batch (default: 10)
 *
 * This class provides a compile-time enforced read-only view of a table.
 * All mutation methods (insert, update, remove, clear) are deleted.
 * Used for system catalog tables to prevent accidental modification.
 *
 * Example:
 * @code
 * ReadOnlyTable<TableMetadata> tables(std::move(sys_tables));
 * auto large = tables.where(TableMetadata::record_count > 1000);
 * // tables.insert(meta); // Compile error!
 * @endcode
 */
template<typename T, std::size_t BatchSize = 10>
requires concepts::Queryable<T, serialization::BinaryWriter, serialization::BinaryReader>
class ReadOnlyTable {
public:
    using value_type = T;
    using primary_key_type = typename T::primary_key_type;

    /**
     * @brief Construct read-only table from storage parameters
     * @param storage Shared pointer to storage engine
     * @param table_name Name of the table
     * @param root_page_id Root page ID for the B-tree index (0 for new table)
     */
    explicit ReadOnlyTable(std::shared_ptr<storage::StorageEngine> storage,
                          std::string table_name,
                          uint64_t root_page_id = 0)
        : table_(std::make_unique<Table<T, BatchSize>>(storage, std::move(table_name), root_page_id)) {}

    // Disable copy (table is movable only)
    ReadOnlyTable(const ReadOnlyTable&) = delete;
    ReadOnlyTable& operator=(const ReadOnlyTable&) = delete;

    // Enable move
    ReadOnlyTable(ReadOnlyTable&&) noexcept = default;
    ReadOnlyTable& operator=(ReadOnlyTable&&) noexcept = default;

    /**
     * @brief Find record by primary key
     * @param key Primary key to search for
     * @return Optional containing record if found
     */
    std::optional<T> find(const primary_key_type& key) const {
        return table_->find(key);
    }

    /**
     * @brief Check if record with given key exists
     * @param key Primary key to check
     * @return true if record exists
     */
    bool contains(const primary_key_type& key) const {
        return table_->contains(key);
    }

    /**
     * @brief Get all records with lazy batch loading
     * @return ProxyVector for memory-efficient iteration
     */
    ranges::ProxyVector<T, BatchSize> get_all() const {
        return table_->get_all();
    }

    /**
     * @brief Find records matching predicate (lambda function)
     * @tparam Predicate Function type taking const T& and returning bool
     * @param pred Predicate function
     * @return ProxyVector of matching records
     */
    template<typename Predicate>
    requires concepts::Predicate<Predicate, T>
    ranges::ProxyVector<T, BatchSize> find_if(Predicate pred) const {
        return table_->find_if(pred);
    }

    /**
     * @brief Filter records using expression template
     * @tparam ExprType Expression template type
     * @param expr Expression to evaluate
     * @return ProxyVector of matching records
     */
    template<typename ExprType>
    ranges::ProxyVector<T, BatchSize> where(const ExprType& expr) const {
        return table_->where(expr);
    }

    /**
     * @brief Get range view of all records
     * @return QueryView for use with C++20 ranges
     */
    ranges::QueryView<T> view() const {
        return table_->view();
    }

    /**
     * @brief Get range view of records matching expression
     * @tparam ExprType Expression template type
     * @param expr Expression to evaluate
     * @return Filtered QueryView
     */
    template<typename ExprType>
    auto view_where(const ExprType& expr) const {
        return table_->view_where(expr);
    }

    /**
     * @brief Alias for view()
     */
    ranges::QueryView<T> all() const {
        return table_->all();
    }

    /**
     * @brief Get number of records in table
     * @return Record count
     */
    std::size_t size() const {
        return table_->size();
    }

    /**
     * @brief Check if table is empty
     * @return true if no records
     */
    bool empty() const {
        return table_->empty();
    }

    // ===== MUTATION METHODS DELETED (COMPILE-TIME ENFORCEMENT) =====

    /**
     * @brief Insert is not allowed on read-only tables
     */
    void insert(const T&) = delete;

    /**
     * @brief Update is not allowed on read-only tables
     */
    void update(const T&) = delete;

    /**
     * @brief Remove is not allowed on read-only tables
     */
    void remove(const primary_key_type&) = delete;

    /**
     * @brief Clear is not allowed on read-only tables
     */
    void clear() = delete;

private:
    // Allow SystemCatalog to access internal table for updates
    friend class catalog::SystemCatalog;

    /**
     * @brief Get mutable access to internal table (SystemCatalog only)
     * @return Reference to wrapped table
     */
    Table<T, BatchSize>& internal_table() {
        return *table_;
    }

    /**
     * @brief Get const access to internal table (SystemCatalog only)
     * @return Const reference to wrapped table
     */
    const Table<T, BatchSize>& internal_table() const {
        return *table_;
    }

    std::unique_ptr<Table<T, BatchSize>> table_;
};

} // namespace learnql::core

#endif // LEARNQL_CORE_READONLYTABLE_HPP
