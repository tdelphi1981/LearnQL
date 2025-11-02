#ifndef LEARNQL_CONCEPTS_SERIALIZABLE_HPP
#define LEARNQL_CONCEPTS_SERIALIZABLE_HPP

#include <concepts>
#include <type_traits>
#include <string>
#include <vector>
#include <memory>

namespace learnql::concepts {

/**
 * @brief Concept for arithmetic types (integers, floats, etc.)
 * @details Fundamental numeric types that can be serialized directly
 */
template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

/**
 * @brief Concept for string-like types
 * @details Types that behave like std::string
 */
template<typename T>
concept StringLike = std::is_same_v<std::remove_cvref_t<T>, std::string> ||
                      std::is_convertible_v<T, std::string>;

/**
 * @brief Concept for types that have custom serialization methods
 * @details Types must implement serialize() and deserialize() methods
 *
 * Example:
 * @code
 * class MyClass {
 * public:
 *     template<typename Writer>
 *     void serialize(Writer& writer) const;
 *     template<typename Reader>
 *     void deserialize(Reader& reader);
 * };
 * @endcode
 */
template<typename T, typename Writer, typename Reader>
concept CustomSerializable = requires(T obj, const T const_obj, Writer& writer, Reader& reader) {
    { const_obj.serialize(writer) } -> std::same_as<void>;
    { obj.deserialize(reader) } -> std::same_as<void>;
};

/**
 * @brief Concept for types that support stream-based serialization
 * @details Types that have overloaded << and >> operators for binary streams
 */
template<typename T>
concept StreamSerializable = requires(T obj, std::ostream& os, std::istream& is) {
    { os << obj } -> std::convertible_to<std::ostream&>;
    { is >> obj } -> std::convertible_to<std::istream&>;
};

/**
 * @brief Main serialization concept
 * @details Types that can be serialized must satisfy one of:
 *          - Arithmetic types (int, float, etc.)
 *          - String types
 *          - Stream serialization
 *
 * Note: Custom serialization is checked separately in write/read methods
 */
template<typename T>
concept Serializable = Arithmetic<T> ||
                       StringLike<T> ||
                       StreamSerializable<T>;

/**
 * @brief Concept for container types
 * @details Types that have begin(), end(), and size() methods
 */
template<typename T>
concept Container = requires(T container) {
    { container.begin() } -> std::input_or_output_iterator;
    { container.end() } -> std::input_or_output_iterator;
    { container.size() } -> std::convertible_to<std::size_t>;
    typename T::value_type;
};

/**
 * @brief Concept for types that can be compared for equality
 */
template<typename T>
concept Comparable = requires(const T& a, const T& b) {
    { a == b } -> std::convertible_to<bool>;
    { a != b } -> std::convertible_to<bool>;
};

/**
 * @brief Concept for types that can be ordered
 */
template<typename T>
concept Orderable = Comparable<T> && requires(const T& a, const T& b) {
    { a < b } -> std::convertible_to<bool>;
    { a > b } -> std::convertible_to<bool>;
    { a <= b } -> std::convertible_to<bool>;
    { a >= b } -> std::convertible_to<bool>;
};

/**
 * @brief Concept for types that support three-way comparison (C++20 spaceship operator)
 */
template<typename T>
concept ThreeWayComparable = requires(const T& a, const T& b) {
    { a <=> b } -> std::convertible_to<std::partial_ordering>;
};

/**
 * @brief Concept for smart pointer types
 */
template<typename T>
concept SmartPointer = requires(T ptr) {
    typename T::element_type;
    { *ptr } -> std::convertible_to<typename T::element_type&>;
    { ptr.get() } -> std::convertible_to<typename T::element_type*>;
} && (std::is_same_v<T, std::shared_ptr<typename T::element_type>> ||
      std::is_same_v<T, std::unique_ptr<typename T::element_type>>);

} // namespace learnql::concepts

#endif // LEARNQL_CONCEPTS_SERIALIZABLE_HPP
