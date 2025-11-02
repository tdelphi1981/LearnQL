#ifndef LEARNQL_CATALOG_FIELDMETADATA_HPP
#define LEARNQL_CATALOG_FIELDMETADATA_HPP

#include "../query/Field.hpp"
#include "../serialization/BinaryWriter.hpp"
#include "../serialization/BinaryReader.hpp"
#include "../reflection/FieldInfo.hpp"
#include <string>
#include <cstdint>

namespace learnql::catalog {

/**
 * @brief Metadata for a table field/column
 *
 * Stored in the _sys_fields system catalog table.
 * Provides queryable information about all fields in all tables.
 *
 * Example usage:
 * @code
 * auto student_fields = catalog.fields()
 *     .where(FieldMetadata::table == "students");
 * auto pk_fields = catalog.fields()
 *     .where(FieldMetadata::is_pk == true);
 * @endcode
 */
struct FieldMetadata {
    using primary_key_type = uint64_t;

    // ===== Data Members =====

    uint64_t field_id;              ///< Unique field ID (primary key)
    std::string table_name;         ///< Name of the table this field belongs to
    std::string field_name;         ///< Name of the field
    std::string field_type;         ///< C++ type name (e.g., "int", "std::string")
    uint16_t field_order;           ///< Position in struct (0-based)
    bool is_primary_key;            ///< true if this is the primary key field

    // ===== Getters (required for Field template construction) =====

    uint64_t get_field_id() const { return field_id; }
    const std::string& get_table_name() const { return table_name; }
    const std::string& get_field_name() const { return field_name; }
    const std::string& get_field_type() const { return field_type; }
    uint16_t get_field_order() const { return field_order; }
    bool get_is_primary_key() const { return is_primary_key; }

    // ===== Static Fields (pre-defined for user queries) =====

    /**
     * @brief Field for field ID
     * Usage: FieldMetadata::id == 123
     */
    static inline query::Field<FieldMetadata, uint64_t> id{
        "field_id", &FieldMetadata::get_field_id
    };

    /**
     * @brief Field for table name
     * Usage: FieldMetadata::table == "students"
     */
    static inline query::Field<FieldMetadata, std::string> table{
        "table_name", &FieldMetadata::get_table_name
    };

    /**
     * @brief Field for field name
     * Usage: FieldMetadata::name == "id"
     */
    static inline query::Field<FieldMetadata, std::string> name{
        "field_name", &FieldMetadata::get_field_name
    };

    /**
     * @brief Field for field type
     * Usage: FieldMetadata::type == "int"
     */
    static inline query::Field<FieldMetadata, std::string> type{
        "field_type", &FieldMetadata::get_field_type
    };

    /**
     * @brief Field for field order
     * Usage: FieldMetadata::order < 5
     */
    static inline query::Field<FieldMetadata, uint16_t> order{
        "field_order", &FieldMetadata::get_field_order
    };

    /**
     * @brief Field for primary key flag
     * Usage: FieldMetadata::is_pk == true
     */
    static inline query::Field<FieldMetadata, bool> is_pk{
        "is_primary_key", &FieldMetadata::get_is_primary_key
    };

    // ===== Primary Key =====

    uint64_t get_primary_key() const {
        return field_id;
    }

    // ===== Serialization =====

    void serialize(serialization::BinaryWriter& writer) const {
        writer.write(field_id);
        writer.write(table_name);
        writer.write(field_name);
        writer.write(field_type);
        writer.write(field_order);
        writer.write(is_primary_key);
    }

    void deserialize(serialization::BinaryReader& reader) {
        field_id = reader.read<uint64_t>();
        table_name = reader.read_string();
        field_name = reader.read_string();
        field_type = reader.read_string();
        field_order = reader.read<uint16_t>();
        is_primary_key = reader.read<bool>();
    }

    // ===== Comparison (for testing/debugging) =====

    bool operator==(const FieldMetadata& other) const {
        return field_id == other.field_id &&
               table_name == other.table_name &&
               field_name == other.field_name &&
               field_type == other.field_type &&
               field_order == other.field_order &&
               is_primary_key == other.is_primary_key;
    }

    // ===== Reflection (for system catalog bootstrap) =====

    static auto reflect_fields() {
        using namespace learnql::reflection;
        std::vector<FieldInfo> fields;
        fields.push_back(FieldInfo{"field_id", "uint64_t", 0, true});
        fields.push_back(FieldInfo{"table_name", "std::string", 1, false});
        fields.push_back(FieldInfo{"field_name", "std::string", 2, false});
        fields.push_back(FieldInfo{"field_type", "std::string", 3, false});
        fields.push_back(FieldInfo{"field_order", "uint16_t", 4, false});
        fields.push_back(FieldInfo{"is_primary_key", "bool", 5, false});
        return fields;
    }
};

} // namespace learnql::catalog

#endif // LEARNQL_CATALOG_FIELDMETADATA_HPP
