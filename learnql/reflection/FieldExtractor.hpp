#ifndef LEARNQL_REFLECTION_FIELDEXTRACTOR_HPP
#define LEARNQL_REFLECTION_FIELDEXTRACTOR_HPP

#include "FieldInfo.hpp"
#include "../catalog/FieldMetadata.hpp"
#include <vector>
#include <tuple>
#include <concepts>

namespace learnql::reflection {

/**
 * @brief Concept for types that support field reflection
 *
 * A type is Reflectable if it has a static reflect_fields() method that
 * returns a tuple of FieldInfo objects.
 */
template<typename T>
concept Reflectable = requires {
    { T::reflect_fields() };
};

/**
 * @brief Extract field metadata from a reflectable type
 * @tparam T Type to extract fields from (must be Reflectable)
 * @param table_name Name of the table
 * @param start_id Starting field ID for generated metadata
 * @return Vector of FieldMetadata for all fields in T
 *
 * Example:
 * @code
 * class Student {
 *     LEARNQL_PROPERTIES_BEGIN(Student)
 *         LEARNQL_PROPERTY(int, id, PK)
 *         LEARNQL_PROPERTY(std::string, name)
 *         LEARNQL_PROPERTY(double, gpa)
 *     LEARNQL_PROPERTIES_END(
 *         PROP(int, id, PK),
 *         PROP(std::string, name),
 *         PROP(double, gpa)
 *     )
 * };
 *
 * auto fields = extract_field_metadata<Student>("students", 1);
 * // Returns 3 FieldMetadata objects with IDs 1, 2, 3
 * @endcode
 */
template<typename T>
std::vector<catalog::FieldMetadata> extract_field_metadata(
    const std::string& table_name,
    uint64_t start_id = 1
) {
    std::vector<catalog::FieldMetadata> fields;

    if constexpr (Reflectable<T>) {
        uint64_t field_id = start_id;
        auto field_vector = T::reflect_fields();

        // Iterate over the FieldInfo vector
        for (const auto& field_info : field_vector) {
            fields.push_back(catalog::FieldMetadata{
                .field_id = field_id++,
                .table_name = table_name,
                .field_name = field_info.name,
                .field_type = field_info.type,
                .field_order = field_info.order,
                .is_primary_key = field_info.is_primary_key
            });
        }
    }
    // If not Reflectable, return empty vector

    return fields;
}

/**
 * @brief Check if a type supports field reflection at compile-time
 * @tparam T Type to check
 * @return true if T is Reflectable, false otherwise
 */
template<typename T>
constexpr bool is_reflectable() {
    return Reflectable<T>;
}

/**
 * @brief Get number of fields in a reflectable type
 * @tparam T Type to inspect (must be Reflectable)
 * @return Number of fields
 */
template<Reflectable T>
std::size_t field_count() {
    return T::reflect_fields().size();
}

} // namespace learnql::reflection

#endif // LEARNQL_REFLECTION_FIELDEXTRACTOR_HPP
