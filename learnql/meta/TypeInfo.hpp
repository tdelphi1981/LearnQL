#ifndef LEARNQL_META_TYPE_INFO_HPP
#define LEARNQL_META_TYPE_INFO_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <any>
#include <typeindex>
#include <stdexcept>

namespace learnql::meta {

/**
 * @brief Enumeration of supported field types
 */
enum class FieldType {
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    Float,
    Double,
    Bool,
    String,
    Custom
};

/**
 * @brief Converts FieldType to string
 */
[[nodiscard]] inline std::string field_type_to_string(FieldType type) {
    switch (type) {
        case FieldType::Int8: return "int8";
        case FieldType::Int16: return "int16";
        case FieldType::Int32: return "int32";
        case FieldType::Int64: return "int64";
        case FieldType::UInt8: return "uint8";
        case FieldType::UInt16: return "uint16";
        case FieldType::UInt32: return "uint32";
        case FieldType::UInt64: return "uint64";
        case FieldType::Float: return "float";
        case FieldType::Double: return "double";
        case FieldType::Bool: return "bool";
        case FieldType::String: return "string";
        case FieldType::Custom: return "custom";
        default: return "unknown";
    }
}

/**
 * @brief Descriptor for a single field in a type
 */
struct FieldDescriptor {
    std::string name;           ///< Field name
    FieldType type;             ///< Field type
    std::size_t offset;         ///< Offset in bytes from start of object
    std::size_t size;           ///< Size in bytes
    bool is_primary_key;        ///< Whether this field is a primary key

    /**
     * @brief Constructor
     */
    FieldDescriptor(std::string field_name, FieldType field_type,
                   std::size_t field_offset, std::size_t field_size,
                   bool is_pk = false)
        : name(std::move(field_name)),
          type(field_type),
          offset(field_offset),
          size(field_size),
          is_primary_key(is_pk) {}
};

/**
 * @brief Runtime type information for a registered type
 * @details Contains metadata about a type including its fields
 *
 * Example:
 * @code
 * TypeInfo info("Student");
 * info.add_field("id", FieldType::Int32, offsetof(Student, id), sizeof(int), true);
 * info.add_field("name", FieldType::String, offsetof(Student, name), sizeof(std::string));
 * @endcode
 */
class TypeInfo {
public:
    /**
     * @brief Default constructor
     */
    TypeInfo() : type_name_{}, type_size_{0}, fields_{}, primary_key_field_{} {}

    /**
     * @brief Constructs type information
     * @param type_name Name of the type
     * @param type_size Size of the type in bytes
     */
    explicit TypeInfo(std::string type_name, std::size_t type_size = 0)
        : type_name_{std::move(type_name)},
          type_size_{type_size},
          fields_{},
          primary_key_field_{} {}

    /**
     * @brief Adds a field to the type
     * @param name Field name
     * @param type Field type
     * @param offset Offset in bytes
     * @param size Size in bytes
     * @param is_primary_key Whether this is the primary key
     * @return Reference to this for chaining
     */
    TypeInfo& add_field(const std::string& name, FieldType type,
                       std::size_t offset, std::size_t size,
                       bool is_primary_key = false) {
        fields_.emplace_back(name, type, offset, size, is_primary_key);

        if (is_primary_key) {
            if (!primary_key_field_.empty()) {
                throw std::runtime_error("Type already has a primary key: " + primary_key_field_);
            }
            primary_key_field_ = name;
        }

        return *this;
    }

    /**
     * @brief Gets the type name
     */
    [[nodiscard]] const std::string& get_type_name() const noexcept {
        return type_name_;
    }

    /**
     * @brief Gets the type size
     */
    [[nodiscard]] std::size_t get_type_size() const noexcept {
        return type_size_;
    }

    /**
     * @brief Gets all fields
     */
    [[nodiscard]] const std::vector<FieldDescriptor>& get_fields() const noexcept {
        return fields_;
    }

    /**
     * @brief Gets a field by name
     * @throws std::runtime_error if field not found
     */
    [[nodiscard]] const FieldDescriptor& get_field(const std::string& name) const {
        for (const auto& field : fields_) {
            if (field.name == name) {
                return field;
            }
        }
        throw std::runtime_error("Field not found: " + name);
    }

    /**
     * @brief Checks if a field exists
     */
    [[nodiscard]] bool has_field(const std::string& name) const noexcept {
        for (const auto& field : fields_) {
            if (field.name == name) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Gets the primary key field name
     * @return Primary key field name, or empty string if none
     */
    [[nodiscard]] const std::string& get_primary_key() const noexcept {
        return primary_key_field_;
    }

    /**
     * @brief Checks if the type has a primary key
     */
    [[nodiscard]] bool has_primary_key() const noexcept {
        return !primary_key_field_.empty();
    }

private:
    std::string type_name_;                  ///< Type name
    std::size_t type_size_;                  ///< Type size in bytes
    std::vector<FieldDescriptor> fields_;    ///< Field descriptors
    std::string primary_key_field_;          ///< Primary key field name
};

/**
 * @brief Global registry of type information
 * @details Singleton that stores metadata for all registered types
 *
 * Example:
 * @code
 * TypeRegistry::instance().register_type<Student>(
 *     TypeInfo("Student", sizeof(Student))
 *         .add_field("id", FieldType::Int32, offsetof(Student, id), sizeof(int), true)
 *         .add_field("name", FieldType::String, offsetof(Student, name), sizeof(std::string))
 * );
 *
 * auto& info = TypeRegistry::instance().get_type_info<Student>();
 * @endcode
 */
class TypeRegistry {
public:
    /**
     * @brief Gets the singleton instance
     */
    static TypeRegistry& instance() {
        static TypeRegistry registry;
        return registry;
    }

    // Delete copy and move
    TypeRegistry(const TypeRegistry&) = delete;
    TypeRegistry& operator=(const TypeRegistry&) = delete;
    TypeRegistry(TypeRegistry&&) = delete;
    TypeRegistry& operator=(TypeRegistry&&) = delete;

    /**
     * @brief Registers a type with its metadata
     * @tparam T Type to register
     * @param info Type information
     */
    template<typename T>
    void register_type(TypeInfo info) {
        std::type_index index = std::type_index(typeid(T));
        type_info_map_[index] = std::move(info);
    }

    /**
     * @brief Registers a type by name
     * @param type_name Name of the type
     * @param info Type information
     */
    void register_type_by_name(const std::string& type_name, TypeInfo info) {
        name_to_info_map_[type_name] = std::move(info);
    }

    /**
     * @brief Gets type information for a type
     * @tparam T Type to get info for
     * @return Type information
     * @throws std::runtime_error if type not registered
     */
    template<typename T>
    [[nodiscard]] const TypeInfo& get_type_info() const {
        std::type_index index = std::type_index(typeid(T));
        auto it = type_info_map_.find(index);
        if (it == type_info_map_.end()) {
            throw std::runtime_error("Type not registered: " + std::string(typeid(T).name()));
        }
        return it->second;
    }

    /**
     * @brief Gets type information by name
     * @param type_name Name of the type
     * @return Type information
     * @throws std::runtime_error if type not registered
     */
    [[nodiscard]] const TypeInfo& get_type_info_by_name(const std::string& type_name) const {
        auto it = name_to_info_map_.find(type_name);
        if (it == name_to_info_map_.end()) {
            throw std::runtime_error("Type not registered: " + type_name);
        }
        return it->second;
    }

    /**
     * @brief Checks if a type is registered
     * @tparam T Type to check
     */
    template<typename T>
    [[nodiscard]] bool is_registered() const noexcept {
        std::type_index index = std::type_index(typeid(T));
        return type_info_map_.find(index) != type_info_map_.end();
    }

    /**
     * @brief Checks if a type is registered by name
     * @param type_name Name of the type
     */
    [[nodiscard]] bool is_registered(const std::string& type_name) const noexcept {
        return name_to_info_map_.find(type_name) != name_to_info_map_.end();
    }

    /**
     * @brief Gets all registered type names
     */
    [[nodiscard]] std::vector<std::string> get_registered_types() const {
        std::vector<std::string> types;
        types.reserve(name_to_info_map_.size());
        for (const auto& [name, _] : name_to_info_map_) {
            types.push_back(name);
        }
        return types;
    }

    /**
     * @brief Clears all registered types
     */
    void clear() noexcept {
        type_info_map_.clear();
        name_to_info_map_.clear();
    }

private:
    TypeRegistry() = default;

    std::unordered_map<std::type_index, TypeInfo> type_info_map_;     ///< Map from type to info
    std::unordered_map<std::string, TypeInfo> name_to_info_map_;      ///< Map from name to info
};

/**
 * @brief Helper macro to register a type with its fields
 * @details Simplifies type registration
 *
 * Example:
 * @code
 * LEARNQL_REGISTER_TYPE(Student,
 *     LEARNQL_FIELD(id, FieldType::Int32, true),
 *     LEARNQL_FIELD(name, FieldType::String, false)
 * );
 * @endcode
 */
#define LEARNQL_REGISTER_TYPE(Type, ...) \
    do { \
        learnql::meta::TypeInfo info(#Type, sizeof(Type)); \
        __VA_ARGS__ \
        learnql::meta::TypeRegistry::instance().register_type<Type>(std::move(info)); \
        learnql::meta::TypeRegistry::instance().register_type_by_name(#Type, info); \
    } while(0)

/**
 * @brief Helper macro to define a field in type registration
 */
#define LEARNQL_FIELD(Type, FieldName, FieldType, IsPrimaryKey) \
    info.add_field(#FieldName, FieldType, offsetof(Type, FieldName), sizeof(Type::FieldName), IsPrimaryKey);

} // namespace learnql::meta

#endif // LEARNQL_META_TYPE_INFO_HPP
