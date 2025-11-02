#ifndef LEARNQL_CATALOG_TABLEMETADATA_HPP
#define LEARNQL_CATALOG_TABLEMETADATA_HPP

#include "../query/Field.hpp"
#include "../serialization/BinaryWriter.hpp"
#include "../serialization/BinaryReader.hpp"
#include "../reflection/FieldInfo.hpp"
#include <string>
#include <cstdint>

namespace learnql::catalog {

/**
 * @brief Metadata for a database table
 *
 * Stored in the _sys_tables system catalog table.
 * Provides queryable information about all tables in the database.
 *
 * Example usage:
 * @code
 * auto large_tables = catalog.tables()
 *     .where(TableMetadata::record_count > 1000);
 * auto student_meta = catalog.tables()
 *     .where(TableMetadata::name == "students");
 * @endcode
 */
struct TableMetadata {
    using primary_key_type = std::string;

    // ===== Data Members =====

    std::string table_name;         ///< Name of the table (primary key)
    std::string type_name;          ///< C++ type name (from typeid)
    uint64_t index_root_page;       ///< Page ID of B-tree root
    std::size_t record_count;       ///< Number of records (cached)
    uint64_t created_timestamp;     ///< Unix timestamp of creation
    bool is_system_table;           ///< true if system table (e.g., _sys_tables)

    // ===== Getters (required for Field template construction) =====

    const std::string& get_table_name() const { return table_name; }
    const std::string& get_type_name() const { return type_name; }
    uint64_t get_index_root_page() const { return index_root_page; }
    std::size_t get_record_count() const { return record_count; }
    uint64_t get_created_timestamp() const { return created_timestamp; }
    bool get_is_system_table() const { return is_system_table; }

    // ===== Static Fields (pre-defined for user queries) =====

    /**
     * @brief Field for table name
     * Usage: TableMetadata::name == "students"
     */
    static inline query::Field<TableMetadata, std::string> name{
        "table_name", &TableMetadata::get_table_name
    };

    /**
     * @brief Field for C++ type name
     * Usage: TableMetadata::type == "Student"
     */
    static inline query::Field<TableMetadata, std::string> type{
        "type_name", &TableMetadata::get_type_name
    };

    /**
     * @brief Field for index root page ID
     * Usage: TableMetadata::root_page > 0
     */
    static inline query::Field<TableMetadata, uint64_t> root_page{
        "index_root_page", &TableMetadata::get_index_root_page
    };

    /**
     * @brief Field for record count
     * Usage: TableMetadata::records > 1000
     */
    static inline query::Field<TableMetadata, std::size_t> records{
        "record_count", &TableMetadata::get_record_count
    };

    /**
     * @brief Field for creation timestamp
     * Usage: TableMetadata::created > some_timestamp
     */
    static inline query::Field<TableMetadata, uint64_t> created{
        "created_timestamp", &TableMetadata::get_created_timestamp
    };

    /**
     * @brief Field for system table flag
     * Usage: TableMetadata::is_system == false
     */
    static inline query::Field<TableMetadata, bool> is_system{
        "is_system_table", &TableMetadata::get_is_system_table
    };

    // ===== Primary Key =====

    std::string get_primary_key() const {
        return table_name;
    }

    // ===== Serialization =====

    void serialize(serialization::BinaryWriter& writer) const {
        writer.write(table_name);
        writer.write(type_name);
        writer.write(index_root_page);
        writer.write(record_count);
        writer.write(created_timestamp);
        writer.write(is_system_table);
    }

    void deserialize(serialization::BinaryReader& reader) {
        table_name = reader.read_string();
        type_name = reader.read_string();
        index_root_page = reader.read<uint64_t>();
        record_count = reader.read<std::size_t>();
        created_timestamp = reader.read<uint64_t>();
        is_system_table = reader.read<bool>();
    }

    // ===== Comparison (for testing/debugging) =====

    bool operator==(const TableMetadata& other) const {
        return table_name == other.table_name &&
               type_name == other.type_name &&
               index_root_page == other.index_root_page &&
               record_count == other.record_count &&
               created_timestamp == other.created_timestamp &&
               is_system_table == other.is_system_table;
    }

    // ===== Reflection (for system catalog bootstrap) =====

    static auto reflect_fields() {
        using namespace learnql::reflection;
        std::vector<FieldInfo> fields;
        fields.push_back(FieldInfo{"table_name", "std::string", 0, true});
        fields.push_back(FieldInfo{"type_name", "std::string", 1, false});
        fields.push_back(FieldInfo{"index_root_page", "uint64_t", 2, false});
        fields.push_back(FieldInfo{"record_count", "std::size_t", 3, false});
        fields.push_back(FieldInfo{"created_timestamp", "uint64_t", 4, false});
        fields.push_back(FieldInfo{"is_system_table", "bool", 5, false});
        return fields;
    }
};

} // namespace learnql::catalog

#endif // LEARNQL_CATALOG_TABLEMETADATA_HPP
