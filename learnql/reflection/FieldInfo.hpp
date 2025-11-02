#ifndef LEARNQL_REFLECTION_FIELDINFO_HPP
#define LEARNQL_REFLECTION_FIELDINFO_HPP

#include <string>
#include <cstdint>

namespace learnql::reflection {

/**
 * @brief Information about a single field in a struct
 *
 * Used for field reflection to extract schema metadata from C++ types.
 * Users provide this information via the reflect_fields() static method.
 *
 * Example:
 * @code
 * struct Student {
 *     int id;
 *     std::string name;
 *     double gpa;
 *
 *     static auto reflect_fields() {
 *         return std::make_tuple(
 *             FieldInfo{"id", "int", 0, true},
 *             FieldInfo{"name", "std::string", 1, false},
 *             FieldInfo{"gpa", "double", 2, false}
 *         );
 *     }
 * };
 * @endcode
 */
struct FieldInfo {
    std::string name;          ///< Field name (e.g., "id", "name")
    std::string type;          ///< C++ type name (e.g., "int", "std::string")
    uint16_t order;            ///< Position in struct (0-based)
    bool is_primary_key;       ///< true if this is the primary key field

    /**
     * @brief Construct field info
     * @param field_name Name of the field
     * @param field_type C++ type as string
     * @param field_order Position in struct
     * @param is_pk Whether this is the primary key
     */
    FieldInfo(std::string field_name,
              std::string field_type,
              uint16_t field_order,
              bool is_pk = false)
        : name(std::move(field_name)),
          type(std::move(field_type)),
          order(field_order),
          is_primary_key(is_pk) {}
};

} // namespace learnql::reflection

#endif // LEARNQL_REFLECTION_FIELDINFO_HPP
