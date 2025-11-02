#ifndef LEARNQL_CATALOG_INDEX_METADATA_HPP
#define LEARNQL_CATALOG_INDEX_METADATA_HPP

#include "../query/Field.hpp"
#include "../serialization/BinaryWriter.hpp"
#include "../serialization/BinaryReader.hpp"
#include <string>
#include <cstdint>

namespace learnql::catalog {

/**
 * @brief Metadata for secondary indexes in the system catalog
 *
 * This structure stores information about secondary indexes created on tables.
 * It's stored in the special `_sys_indexes` table and enables:
 * - Persistence of index configuration across database restarts
 * - Queryable index metadata using expression templates
 * - Automatic index reconstruction when tables are loaded
 *
 * Example:
 * @code
 * auto& catalog = db.metadata();
 * auto student_indexes = catalog.indexes()
 *     .where(IndexMetadata::table == "students");
 *
 * for (const auto& idx : student_indexes) {
 *     std::cout << "Index on " << idx.field_name
 *               << " (" << (idx.is_unique ? "unique" : "multi-value") << ")\n";
 * }
 * @endcode
 */
struct IndexMetadata {
    using primary_key_type = uint64_t;

    // ===== Data Members =====

    uint64_t index_id;              ///< Unique index ID (auto-incremented, primary key)
    std::string table_name;         ///< Table this index belongs to
    std::string field_name;         ///< Indexed field name
    std::string field_type;         ///< C++ type name (e.g., "std::string", "int")
    bool is_unique;                 ///< true = unique index, false = multi-value
    uint64_t index_root_page;       ///< B+Tree root page ID
    uint64_t created_timestamp;     ///< Creation timestamp
    bool is_active;                 ///< Can be disabled without deletion

    // ===== Getters (required for Field template construction) =====

    uint64_t get_index_id() const { return index_id; }
    const std::string& get_table_name() const { return table_name; }
    const std::string& get_field_name() const { return field_name; }
    const std::string& get_field_type() const { return field_type; }
    bool get_is_unique() const { return is_unique; }
    uint64_t get_index_root_page() const { return index_root_page; }
    uint64_t get_created_timestamp() const { return created_timestamp; }
    bool get_is_active() const { return is_active; }

    // ===== Static Fields (pre-defined for user queries) =====

    /**
     * @brief Field for index ID
     * Usage: IndexMetadata::id == 1
     */
    static inline query::Field<IndexMetadata, uint64_t> id{
        "index_id", &IndexMetadata::get_index_id
    };

    /**
     * @brief Field for table name
     * Usage: IndexMetadata::table == "students"
     */
    static inline query::Field<IndexMetadata, std::string> table{
        "table_name", &IndexMetadata::get_table_name
    };

    /**
     * @brief Field for field name
     * Usage: IndexMetadata::field == "name"
     */
    static inline query::Field<IndexMetadata, std::string> field{
        "field_name", &IndexMetadata::get_field_name
    };

    /**
     * @brief Field for field type
     * Usage: IndexMetadata::type == "std::string"
     */
    static inline query::Field<IndexMetadata, std::string> type{
        "field_type", &IndexMetadata::get_field_type
    };

    /**
     * @brief Field for unique flag
     * Usage: IndexMetadata::unique == true
     */
    static inline query::Field<IndexMetadata, bool> unique{
        "is_unique", &IndexMetadata::get_is_unique
    };

    /**
     * @brief Field for index root page
     * Usage: IndexMetadata::root_page > 0
     */
    static inline query::Field<IndexMetadata, uint64_t> root_page{
        "index_root_page", &IndexMetadata::get_index_root_page
    };

    /**
     * @brief Field for creation timestamp
     * Usage: IndexMetadata::created > some_timestamp
     */
    static inline query::Field<IndexMetadata, uint64_t> created{
        "created_timestamp", &IndexMetadata::get_created_timestamp
    };

    /**
     * @brief Field for active status
     * Usage: IndexMetadata::active == true
     */
    static inline query::Field<IndexMetadata, bool> active{
        "is_active", &IndexMetadata::get_is_active
    };

    // ===== Primary Key =====

    /**
     * @brief Get primary key value for this metadata record
     * @return The index_id (primary key)
     */
    uint64_t get_primary_key() const {
        return index_id;
    }

    /**
     * @brief Set primary key value
     * @param key The new primary key value
     */
    void set_primary_key(uint64_t key) {
        index_id = key;
    }

    // ===== Serialization =====

    /**
     * @brief Serialize to binary writer
     */
    void serialize(serialization::BinaryWriter& writer) const {
        writer.write(index_id);
        writer.write(table_name);
        writer.write(field_name);
        writer.write(field_type);
        writer.write(is_unique);
        writer.write(index_root_page);
        writer.write(created_timestamp);
        writer.write(is_active);
    }

    /**
     * @brief Deserialize from binary reader
     */
    void deserialize(serialization::BinaryReader& reader) {
        index_id = reader.read<uint64_t>();
        table_name = reader.read_string();
        field_name = reader.read_string();
        field_type = reader.read_string();
        is_unique = reader.read<bool>();
        index_root_page = reader.read<uint64_t>();
        created_timestamp = reader.read<uint64_t>();
        is_active = reader.read<bool>();
    }
};

} // namespace learnql::catalog

#endif // LEARNQL_CATALOG_INDEX_METADATA_HPP
