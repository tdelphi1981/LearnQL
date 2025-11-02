#ifndef LEARNQL_QUERY_EXPRESSIONS_FIELD_EXPR_HPP
#define LEARNQL_QUERY_EXPRESSIONS_FIELD_EXPR_HPP

#include "Expr.hpp"
#include <functional>
#include <string>

namespace learnql::query::expressions {

/**
 * @brief Expression representing a field access
 * @tparam T Object type
 * @tparam FieldType Type of the field
 *
 * Example:
 * @code
 * FieldExpr<Student, int> age_field{"age", &Student::getAge};
 * auto result = age_field.evaluate(student);  // Calls student.getAge()
 * @endcode
 */
template<typename T, typename FieldType>
class FieldExpr : public Expr<FieldExpr<T, FieldType>> {
public:
    using object_type = T;
    using value_type = FieldType;
    using getter_type = std::function<FieldType(const T&)>;

    /**
     * @brief Constructs a field expression from a member function pointer
     * @param name Field name (for debugging)
     * @param getter Member function pointer
     */
    template<typename MemberFunc>
    FieldExpr(std::string name, MemberFunc getter)
        : name_(std::move(name)),
          getter_([getter](const T& obj) -> FieldType {
              if constexpr (std::is_member_function_pointer_v<MemberFunc>) {
                  return (obj.*getter)();
              } else {
                  return getter(obj);
              }
          }) {}

    /**
     * @brief Constructs a field expression from a getter function
     * @param name Field name
     * @param getter Getter function
     */
    FieldExpr(std::string name, getter_type getter)
        : name_(std::move(name)),
          getter_(std::move(getter)) {}

    /**
     * @brief Evaluates the field for a given object
     * @param obj Object to extract field from
     * @return Field value
     */
    [[nodiscard]] FieldType evaluate(const T& obj) const {
        return getter_(obj);
    }

    /**
     * @brief Gets the field name
     */
    [[nodiscard]] const std::string& name() const noexcept {
        return name_;
    }

    /**
     * @brief Converts to string representation
     */
    [[nodiscard]] std::string to_string() const {
        return name_;
    }

    /**
     * @brief Gets the getter function (for advanced use)
     */
    [[nodiscard]] const getter_type& getter() const noexcept {
        return getter_;
    }

private:
    std::string name_;
    getter_type getter_;
};

// Deduction guides
template<typename T, typename FieldType>
FieldExpr(std::string, FieldType (T::*)() const) -> FieldExpr<T, FieldType>;

template<typename T, typename FieldType>
FieldExpr(std::string, std::function<FieldType(const T&)>) -> FieldExpr<T, FieldType>;

} // namespace learnql::query::expressions

#endif // LEARNQL_QUERY_EXPRESSIONS_FIELD_EXPR_HPP
