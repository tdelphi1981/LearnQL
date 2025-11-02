#ifndef LEARNQL_META_PROPERTY_HPP
#define LEARNQL_META_PROPERTY_HPP

#include "../query/Field.hpp"
#include "../reflection/FieldInfo.hpp"
#include <type_traits>
#include <string>
#include <tuple>
#include <vector>

namespace learnql::meta {

/**
 * @brief Type trait to determine the appropriate return type for a property getter
 *
 * Small, trivially copyable types (like int, double) are returned by value.
 * Larger types (like std::string) are returned by const reference.
 *
 * @tparam T The property type
 */
template<typename T>
using property_return_type = std::conditional_t<
    std::is_fundamental_v<T> || (sizeof(T) <= sizeof(void*) && std::is_trivially_copyable_v<T>),
    T,           // Return by value for small types
    const T&     // Return by const& for large types
>;

/**
 * @brief Type trait to determine the appropriate parameter type for a property setter
 *
 * Fundamental types are passed by value.
 * Non-fundamental types are passed by const reference.
 *
 * @tparam T The property type
 */
template<typename T>
using property_param_type = std::conditional_t<
    std::is_fundamental_v<T>,
    T,           // Pass by value for fundamentals
    const T&     // Pass by const& for objects
>;

/**
 * @brief Helper function to deserialize a property based on its type
 *
 * Dispatches to the appropriate BinaryReader method:
 * - std::string -> read_string()
 * - Arithmetic types -> read<T>()
 * - Custom types -> read_custom<T>()
 *
 * @tparam T The property type
 * @tparam Reader The reader type
 * @param reader The binary reader
 * @return The deserialized value
 */
template<typename T, typename Reader>
T deserialize_property(Reader& reader) {
    if constexpr (std::is_same_v<T, std::string>) {
        return reader.read_string();
    } else if constexpr (std::is_arithmetic_v<T>) {
        return reader.template read<T>();
    } else {
        // For custom types that have serialize/deserialize methods
        return reader.template read_custom<T>();
    }
}

/**
 * @brief Compile-time type name helper
 * Maps C++ types to string representations for reflection
 */
template<typename T>
constexpr const char* type_name() {
    if constexpr (std::is_same_v<T, int>) return "int";
    else if constexpr (std::is_same_v<T, unsigned int>) return "unsigned int";
    else if constexpr (std::is_same_v<T, long>) return "long";
    else if constexpr (std::is_same_v<T, unsigned long>) return "unsigned long";
    else if constexpr (std::is_same_v<T, long long>) return "long long";
    else if constexpr (std::is_same_v<T, unsigned long long>) return "unsigned long long";
    else if constexpr (std::is_same_v<T, short>) return "short";
    else if constexpr (std::is_same_v<T, unsigned short>) return "unsigned short";
    else if constexpr (std::is_same_v<T, char>) return "char";
    else if constexpr (std::is_same_v<T, unsigned char>) return "unsigned char";
    else if constexpr (std::is_same_v<T, bool>) return "bool";
    else if constexpr (std::is_same_v<T, float>) return "float";
    else if constexpr (std::is_same_v<T, double>) return "double";
    else if constexpr (std::is_same_v<T, long double>) return "long double";
    else if constexpr (std::is_same_v<T, std::string>) return "std::string";
    else return "unknown";
}

/**
 * @brief Compile-time property metadata
 * Stores information about a single property for reflection
 */
template<typename Class, typename Type, bool IsPK = false>
struct PropertyMeta {
    const char* name;
    Type Class::*member_ptr;
    uint16_t index;

    static constexpr bool is_primary_key = IsPK;
    using value_type = Type;
    using class_type = Class;

    // Get property value from object instance
    constexpr auto get(const Class& obj) const -> property_return_type<Type> {
        return obj.*member_ptr;
    }

    // Set property value on object instance
    constexpr void set(Class& obj, property_param_type<Type> value) const {
        obj.*member_ptr = value;
    }

    // Get compile-time type name
    static constexpr const char* type_string() {
        return type_name<Type>();
    }
};

} // namespace learnql::meta

// ============================================================================
// NEW PROPERTY MACRO SYSTEM - Single Definition, Everything Auto-Generated
// ============================================================================

/**
 * @brief Begin property block for a class
 *
 * Usage:
 * @code
 * class Student {
 *     LEARNQL_PROPERTIES_BEGIN(Student)
 *         LEARNQL_PROPERTY(int, student_id, PK)
 *         LEARNQL_PROPERTY(std::string, name)
 *     LEARNQL_PROPERTIES_END(
 *         PROP(int, student_id, PK),
 *         PROP(std::string, name)
 *     )
 * };
 * @endcode
 */
#define LEARNQL_PROPERTIES_BEGIN(ClassName) \
private: \
    enum { _LEARNQL_PROP_COUNTER_START = __COUNTER__ }; \
public:

/**
 * @brief Define a single property
 *
 * Generates:
 * - Private member variable (Type name_)
 * - Public getter (get_name())
 * - Public setter (set_name())
 * - Static Field object for queries (name)
 *
 * @param Type The property type
 * @param name The property name
 * @param ... Optional: PK to mark as primary key
 */
#define LEARNQL_PROPERTY(Type, name, ...) \
private: \
    Type name##_; \
public: \
    [[nodiscard]] ::learnql::meta::property_return_type<Type> get_##name() const { \
        return name##_; \
    } \
    void set_##name(::learnql::meta::property_param_type<Type> value) { \
        name##_ = value; \
    } \
    static inline const ::learnql::query::Field<_LEARNQL_CLASS, Type> name{ \
        #name, &_LEARNQL_CLASS::get_##name \
    };

/**
 * @brief Helper macro to detect if PK flag is present
 */
#define _LEARNQL_IS_PK_3(a, b, c) true
#define _LEARNQL_IS_PK_2(a, b) false
#define _LEARNQL_IS_PK_1(a) false
#define _LEARNQL_GET_4TH_ARG(a, b, c, d, ...) d
#define _LEARNQL_IS_PK(...) \
    _LEARNQL_GET_4TH_ARG(__VA_ARGS__, _LEARNQL_IS_PK_3, _LEARNQL_IS_PK_2, _LEARNQL_IS_PK_1)(__VA_ARGS__)

/**
 * @brief Property descriptor for PROPERTIES_END
 * Each property must be listed here to enable code generation
 */
#define PROP(Type, name, ...) \
    ::learnql::meta::PropertyMeta<_LEARNQL_CLASS, Type, _LEARNQL_IS_PK(Type, name __VA_OPT__(,) __VA_ARGS__)>{ \
        #name, \
        &_LEARNQL_CLASS::name##_, \
        static_cast<uint16_t>(__COUNTER__ - _LEARNQL_PROP_COUNTER_START - 1) \
    }

/**
 * @brief End property block and generate all boilerplate code
 *
 * Auto-generates:
 * - primary_key_type typedef
 * - get_primary_key() method
 * - _properties() tuple
 * - serialize() method
 * - deserialize() method
 * - reflect_fields() for system catalog
 *
 * @param ... List of PROP() descriptors for all properties
 */
#define LEARNQL_PROPERTIES_END(...) \
private: \
    /* Helper to get first property's type for primary key detection */ \
    using _first_prop_type = std::tuple_element_t<0, decltype(std::make_tuple(__VA_ARGS__))>; \
    \
public: \
    /* Primary key type typedef - from first property marked as PK */ \
    using primary_key_type = typename _first_prop_type::value_type; \
    \
    /* Primary key getter */ \
    primary_key_type get_primary_key() const { \
        constexpr auto props = std::make_tuple(__VA_ARGS__); \
        primary_key_type pk{}; \
        std::apply([this, &pk](const auto&... p) { \
            (([this, &pk](const auto& prop) { \
                if constexpr (std::remove_cvref_t<decltype(prop)>::is_primary_key) { \
                    pk = prop.get(*this); \
                } \
            }(p)), ...); \
        }, props); \
        return pk; \
    } \
    \
    /* Property metadata tuple (compile-time) */ \
    static constexpr auto _properties() { \
        return std::make_tuple(__VA_ARGS__); \
    } \
    \
    /* Auto-generated serialization */ \
    template<typename Writer> \
    void serialize(Writer& writer) const { \
        constexpr auto props = _properties(); \
        std::apply([this, &writer](const auto&... p) { \
            ((writer.write(p.get(*this))), ...); \
        }, props); \
    } \
    \
    /* Auto-generated deserialization */ \
    template<typename Reader> \
    void deserialize(Reader& reader) { \
        constexpr auto props = _properties(); \
        std::apply([this, &reader](const auto&... p) { \
            ((p.set(*this, ::learnql::meta::deserialize_property< \
                std::remove_cvref_t<decltype(p.get(*this))> \
            >(reader))), ...); \
        }, props); \
    } \
    \
    /* Auto-generated reflection for system catalog */ \
    static auto reflect_fields() { \
        using namespace learnql::reflection; \
        std::vector<FieldInfo> fields; \
        constexpr auto props = _properties(); \
        std::apply([&fields](const auto&... p) { \
            ((fields.push_back(FieldInfo{ \
                p.name, \
                std::remove_cvref_t<decltype(p)>::type_string(), \
                p.index, \
                std::remove_cvref_t<decltype(p)>::is_primary_key \
            })), ...); \
        }, props); \
        return fields; \
    }

// Store class name for use in LEARNQL_PROPERTY
#define _LEARNQL_CLASS _LearnQL_Current_Class

// Redefine PROPERTIES_BEGIN to also set the class name
#undef LEARNQL_PROPERTIES_BEGIN
#define LEARNQL_PROPERTIES_BEGIN(ClassName) \
    using _LearnQL_Current_Class = ClassName; \
private: \
    enum { _LEARNQL_PROP_COUNTER_START = __COUNTER__ }; \
public:

#endif // LEARNQL_META_PROPERTY_HPP
