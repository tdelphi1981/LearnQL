#ifndef LEARNQL_CONCEPTS_QUERYABLE_HPP
#define LEARNQL_CONCEPTS_QUERYABLE_HPP

#include "Serializable.hpp"
#include <concepts>
#include <type_traits>
#include <functional>

namespace learnql::concepts {

/**
 * @brief Concept for types that have a primary key
 * @details Types must have a member type 'primary_key_type' and a method to get the key
 *
 * Example:
 * @code
 * class Student {
 * public:
 *     using primary_key_type = int;
 *     int get_primary_key() const { return id; }
 * private:
 *     int id;
 * };
 * @endcode
 */
template<typename T>
concept HasPrimaryKey = requires(const T& obj) {
    typename T::primary_key_type;
    { obj.get_primary_key() } -> std::convertible_to<typename T::primary_key_type>;
};

/**
 * @brief Concept for types that can be used as primary keys
 * @details Keys must be comparable and hashable
 */
template<typename T>
concept PrimaryKeyType = Comparable<T> &&
                         std::is_default_constructible_v<T> &&
                         std::is_copy_constructible_v<T> &&
                         requires(const T& key) {
    { std::hash<T>{}(key) } -> std::convertible_to<std::size_t>;
};

/**
 * @brief Concept for types that can be stored in a table
 * @details Types must:
 *          - Be default constructible
 *          - Be copy constructible
 *          - Have a primary key
 *          - Have custom serialization
 *
 * This is the main concept for domain objects that can be stored in LearnQL tables.
 */
template<typename T, typename Writer, typename Reader>
concept Storable = std::is_default_constructible_v<T> &&
                   std::is_copy_constructible_v<T> &&
                   HasPrimaryKey<T> &&
                   CustomSerializable<T, Writer, Reader> &&
                   PrimaryKeyType<typename T::primary_key_type>;

/**
 * @brief Concept for predicate functions
 * @details Functions that take an object and return bool
 */
template<typename F, typename T>
concept Predicate = std::is_invocable_r_v<bool, F, const T&>;

/**
 * @brief Concept for types that can be indexed
 * @details Types whose primary key is orderable
 */
template<typename T>
concept Indexable = HasPrimaryKey<T> &&
                    Orderable<typename T::primary_key_type>;

/**
 * @brief Concept for types that can be queried
 * @details Combination of Storable and Indexable
 */
template<typename T, typename Writer, typename Reader>
concept Queryable = Storable<T, Writer, Reader> && Indexable<T>;

/**
 * @brief Concept for iterator types
 */
template<typename It>
concept Iterator = requires(It it) {
    { *it } -> std::convertible_to<typename std::iterator_traits<It>::reference>;
    { ++it } -> std::same_as<It&>;
    { it++ } -> std::convertible_to<It>;
};

/**
 * @brief Concept for forward iterator types
 */
template<typename It>
concept ForwardIterator = Iterator<It> && requires(It it1, It it2) {
    { it1 == it2 } -> std::convertible_to<bool>;
    { it1 != it2 } -> std::convertible_to<bool>;
};

} // namespace learnql::concepts

#endif // LEARNQL_CONCEPTS_QUERYABLE_HPP
