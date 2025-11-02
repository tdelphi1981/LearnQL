#ifndef LEARNQL_CATALOG_SYSTEMCATALOG_HPP
#define LEARNQL_CATALOG_SYSTEMCATALOG_HPP

#include "TableMetadata.hpp"
#include "FieldMetadata.hpp"
#include "IndexMetadata.hpp"
#include "../core/ReadOnlyTable.hpp"
#include "../core/Table.hpp"
#include "../storage/StorageEngine.hpp"
#include <memory>
#include <vector>
#include <stdexcept>
#include <chrono>

namespace learnql {
    // Forward declarations
    namespace core {
        class Database;
    }
}

namespace learnql::catalog {

/**
 * @brief System catalog providing queryable metadata about tables, fields, and indexes
 *
 * The system catalog stores metadata in three special tables:
 * - _sys_tables: Information about all tables
 * - _sys_fields: Information about all fields/columns
 * - _sys_indexes: Information about secondary indexes (NEW!)
 *
 * These tables are queryable using the same expression template and lambda
 * query interface as regular tables, but are read-only to prevent corruption.
 *
 * Example usage:
 * @code
 * Database db("myapp.db");
 * auto& catalog = db.metadata();
 *
 * // Query tables
 * auto large = catalog.tables()
 *     .where(TableMetadata::record_count > 1000);
 *
 * // Query fields
 * auto student_fields = catalog.fields()
 *     .where(FieldMetadata::table == "students");
 *
 * // Query indexes (NEW!)
 * auto student_indexes = catalog.indexes()
 *     .where(IndexMetadata::table == "students");
 *
 * // Use with ranges
 * auto top = catalog.tables().view()
 *     | order_by(&TableMetadata::get_record_count, false)
 *     | limit(10);
 * @endcode
 */
class SystemCatalog {
public:
    /**
     * @brief Construct system catalog by loading existing system tables
     * @param storage Storage engine
     * @param sys_tables_root Root page ID for _sys_tables
     * @param sys_fields_root Root page ID for _sys_fields
     * @param sys_indexes_root Root page ID for _sys_indexes
     *
     * Used when opening an existing database.
     */
    SystemCatalog(
        std::shared_ptr<storage::StorageEngine> storage,
        uint64_t sys_tables_root,
        uint64_t sys_fields_root,
        uint64_t sys_indexes_root
    ) : storage_(storage),
        tables_table_(storage, "_sys_tables", sys_tables_root),
        fields_table_(storage, "_sys_fields", sys_fields_root),
        indexes_table_(storage, "_sys_indexes", sys_indexes_root),
        next_field_id_(1),  // Temporary value, updated in constructor body
        next_index_id_(1)   // Temporary value, updated in constructor body
    {
        // Compute next field ID after fields_table_ is constructed
        next_field_id_ = compute_next_field_id();
        next_index_id_ = compute_next_index_id();
    }

    // Note: The second constructor is removed - use the first constructor for both cases

    // Disable copy
    SystemCatalog(const SystemCatalog&) = delete;
    SystemCatalog& operator=(const SystemCatalog&) = delete;

    // Enable move
    SystemCatalog(SystemCatalog&&) noexcept = default;
    SystemCatalog& operator=(SystemCatalog&&) noexcept = default;

    // ===== Public Read-Only API =====

    /**
     * @brief Get read-only access to tables metadata
     * @return ReadOnlyTable for querying table metadata
     */
    const core::ReadOnlyTable<TableMetadata>& tables() const {
        return tables_table_;
    }

    /**
     * @brief Get read-only access to fields metadata
     * @return ReadOnlyTable for querying field metadata
     */
    const core::ReadOnlyTable<FieldMetadata>& fields() const {
        return fields_table_;
    }

    /**
     * @brief Get read-only access to indexes metadata (NEW!)
     * @return ReadOnlyTable for querying index metadata
     *
     * Example:
     * @code
     * auto student_indexes = catalog.indexes()
     *     .where(IndexMetadata::table == "students");
     *
     * for (const auto& idx : student_indexes) {
     *     std::cout << idx.field_name << " ("
     *               << (idx.is_unique ? "unique" : "multi-value") << ")\n";
     * }
     * @endcode
     */
    const core::ReadOnlyTable<IndexMetadata>& indexes() const {
        return indexes_table_;
    }

private:
    // Only Database and Table classes can modify system catalog
    friend class core::Database;

    template<typename T, std::size_t BatchSize>
    requires concepts::Queryable<T, serialization::BinaryWriter, serialization::BinaryReader>
    friend class core::Table;

    // ===== Internal Mutation API (Database only) =====

    /**
     * @brief Register a new table in the catalog
     * @param meta Table metadata
     * @param field_metas Field metadata for the table
     * @throws std::runtime_error if table already exists
     */
    void register_table(
        const TableMetadata& meta,
        const std::vector<FieldMetadata>& field_metas
    ) {
        // Check if table already exists
        if (tables_table_.internal_table().contains(meta.table_name)) {
            throw std::runtime_error(
                "Table '" + meta.table_name + "' already registered in catalog"
            );
        }

        // Insert table metadata
        tables_table_.internal_table().insert(meta);

        // Insert all field metadata
        for (auto field_meta : field_metas) {
            // Assign unique field ID
            field_meta.field_id = next_field_id_++;
            fields_table_.internal_table().insert(field_meta);
        }
    }

    /**
     * @brief Unregister a table from the catalog
     * @param table_name Name of table to remove
     *
     * Removes table metadata and all associated field metadata.
     */
    void unregister_table(const std::string& table_name) {
        // Remove table metadata
        tables_table_.internal_table().remove(table_name);

        // Remove all field metadata for this table
        auto& fields_table = fields_table_.internal_table();
        auto all_fields = fields_table.get_all();

        for (const auto& field : all_fields) {
            if (field.table_name == table_name) {
                fields_table.remove(field.field_id);
            }
        }
    }

    /**
     * @brief Update record count for a table
     * @param table_name Name of table
     * @param count New record count
     */
    void update_record_count(const std::string& table_name, std::size_t count) {
        auto meta_opt = tables_table_.internal_table().find(table_name);
        if (!meta_opt) {
            // Table not in catalog - might be system table during bootstrap
            return;
        }

        auto meta = *meta_opt;
        meta.record_count = count;
        tables_table_.internal_table().update(meta);
    }

    // ===== Secondary Index Management (NEW!) =====

    /**
     * @brief Register a new secondary index in the catalog
     * @param table_name Name of the table
     * @param field_name Name of the indexed field
     * @param field_type C++ type name of the field
     * @param is_unique true for unique index, false for multi-value
     * @param root_page_id Root page ID of the index B+Tree
     * @return The assigned index ID
     */
    uint64_t register_index(
        const std::string& table_name,
        const std::string& field_name,
        const std::string& field_type,
        bool is_unique,
        uint64_t root_page_id
    ) {
        auto timestamp = static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );

        IndexMetadata meta(
            next_index_id_++,
            table_name,
            field_name,
            field_type,
            is_unique,
            root_page_id,
            timestamp,
            true  // is_active
        );

        indexes_table_.internal_table().insert(meta);
        return meta.get_index_id();
    }

    /**
     * @brief Unregister a secondary index from the catalog
     * @param table_name Name of the table
     * @param field_name Name of the indexed field
     * @return true if index was found and removed
     */
    bool unregister_index(const std::string& table_name, const std::string& field_name) {
        auto& indexes_table = indexes_table_.internal_table();
        auto all_indexes = indexes_table.get_all();

        for (const auto& idx : all_indexes) {
            if (idx.get_table_name() == table_name && idx.get_field_name() == field_name) {
                return indexes_table.remove(idx.get_index_id());
            }
        }

        return false;
    }

    /**
     * @brief Get all indexes for a specific table
     * @param table_name Name of the table
     * @return Vector of index metadata
     */
    std::vector<IndexMetadata> get_table_indexes(const std::string& table_name) const {
        std::vector<IndexMetadata> result;

        // Early return if indexes table is empty (no indexes registered yet)
        if (indexes_table_.empty()) {
            return result;
        }

        auto all_indexes = indexes_table_.internal_table().get_all();

        for (const auto& idx : all_indexes) {
            if (idx.get_table_name() == table_name) {
                result.push_back(idx);
            }
        }

        return result;
    }

    /**
     * @brief Get root page ID for tables table
     */
    uint64_t get_tables_root_page() const {
        return tables_table_.internal_table().get_root_page();
    }

    /**
     * @brief Get root page ID for fields table
     */
    uint64_t get_fields_root_page() const {
        return fields_table_.internal_table().get_root_page();
    }

    /**
     * @brief Get root page ID for indexes table (NEW!)
     */
    uint64_t get_indexes_root_page() const {
        return indexes_table_.internal_table().get_root_page();
    }

    /**
     * @brief Compute next available field ID from existing fields
     * @return Next field ID to use
     */
    uint64_t compute_next_field_id() {
        // If table is empty, start from 1
        if (fields_table_.empty()) {
            return 1;
        }

        uint64_t max_id = 0;
        auto all_fields = fields_table_.internal_table().get_all();

        for (const auto& field : all_fields) {
            if (field.field_id > max_id) {
                max_id = field.field_id;
            }
        }

        return max_id + 1;
    }

    /**
     * @brief Compute next available index ID from existing indexes
     * @return Next index ID to use
     */
    uint64_t compute_next_index_id() {
        // If table is empty, start from 1
        if (indexes_table_.empty()) {
            return 1;
        }

        uint64_t max_id = 0;
        auto all_indexes = indexes_table_.internal_table().get_all();

        for (const auto& idx : all_indexes) {
            if (idx.get_index_id() > max_id) {
                max_id = idx.get_index_id();
            }
        }

        return max_id + 1;
    }

    std::shared_ptr<storage::StorageEngine> storage_;
    core::ReadOnlyTable<TableMetadata> tables_table_;
    core::ReadOnlyTable<FieldMetadata> fields_table_;
    core::ReadOnlyTable<IndexMetadata> indexes_table_;  // NEW!
    uint64_t next_field_id_;   // Auto-increment for field IDs
    uint64_t next_index_id_;   // Auto-increment for index IDs
};

} // namespace learnql::catalog

#endif // LEARNQL_CATALOG_SYSTEMCATALOG_HPP
