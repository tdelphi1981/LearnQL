#ifndef LEARNQL_QUERY_FIELD_HPP
#define LEARNQL_QUERY_FIELD_HPP

#include "expressions/FieldExpr.hpp"
#include "expressions/ConstExpr.hpp"
#include "expressions/BinaryExpr.hpp"
#include "expressions/LogicalExpr.hpp"
#include <string>
#include <functional>

namespace learnql::query {

using namespace expressions;

/**
 * @brief User-facing field proxy for building queries
 * @tparam T Object type
 * @tparam FieldType Type of the field
 *
 * This is the main API for building SQL-like queries. It provides operator
 * overloads that create expression templates.
 *
 * Example:
 * @code
 * Field<Student, int> age{"age", &Student::getAge};
 * Field<Student, std::string> name{"name", &Student::getName};
 *
 * auto query = (age > 18) && (name == "Alice");
 * // Creates an expression tree without evaluating
 * @endcode
 */
template<typename T, typename FieldType>
class Field {
public:
    using field_expr_type = FieldExpr<T, FieldType>;

    /**
     * @brief Constructs a field from a member function pointer
     * @param name Field name (for debugging)
     * @param getter Member function pointer
     */
    template<typename MemberFunc>
    Field(std::string name, MemberFunc getter)
        : expr_(std::move(name), getter) {}

    /**
     * @brief Constructs a field from a getter function
     * @param name Field name
     * @param getter Getter function
     */
    Field(std::string name, std::function<FieldType(const T&)> getter)
        : expr_(std::move(name), std::move(getter)) {}

    /**
     * @brief Gets the underlying field expression
     */
    [[nodiscard]] const field_expr_type& expr() const noexcept {
        return expr_;
    }

    /**
     * @brief Implicit conversion to field expression
     */
    operator const field_expr_type&() const noexcept {
        return expr_;
    }

    // Comparison operators

    /**
     * @brief Equality comparison with C-string literal: field == "value"
     */
    [[nodiscard]] auto operator==(const char* value) const
    requires std::is_same_v<FieldType, std::string> {
        return BinaryExpr<BinaryOp::Equal, field_expr_type, ConstExpr<std::string>>{
            expr_, ConstExpr<std::string>{std::string(value)}
        };
    }

    /**
     * @brief Equality comparison: field == value
     */
    template<typename U>
    [[nodiscard]] auto operator==(const U& value) const {
        return BinaryExpr<BinaryOp::Equal, field_expr_type, ConstExpr<U>>{
            expr_, ConstExpr<U>{value}
        };
    }

    /**
     * @brief Field-to-field equality: field == other_field
     */
    template<typename U>
    [[nodiscard]] auto operator==(const Field<T, U>& other) const {
        return BinaryExpr<BinaryOp::Equal, field_expr_type, FieldExpr<T, U>>{
            expr_, other.expr()
        };
    }

    /**
     * @brief Inequality comparison with C-string literal: field != "value"
     */
    [[nodiscard]] auto operator!=(const char* value) const
    requires std::is_same_v<FieldType, std::string> {
        return BinaryExpr<BinaryOp::NotEqual, field_expr_type, ConstExpr<std::string>>{
            expr_, ConstExpr<std::string>{std::string(value)}
        };
    }

    /**
     * @brief Inequality comparison: field != value
     */
    template<typename U>
    [[nodiscard]] auto operator!=(const U& value) const {
        return BinaryExpr<BinaryOp::NotEqual, field_expr_type, ConstExpr<U>>{
            expr_, ConstExpr<U>{value}
        };
    }

    /**
     * @brief Field-to-field inequality: field != other_field
     */
    template<typename U>
    [[nodiscard]] auto operator!=(const Field<T, U>& other) const {
        return BinaryExpr<BinaryOp::NotEqual, field_expr_type, FieldExpr<T, U>>{
            expr_, other.expr()
        };
    }

    /**
     * @brief Less-than comparison: field < value
     */
    template<typename U>
    [[nodiscard]] auto operator<(const U& value) const {
        return BinaryExpr<BinaryOp::Less, field_expr_type, ConstExpr<U>>{
            expr_, ConstExpr<U>{value}
        };
    }

    /**
     * @brief Field-to-field less-than: field < other_field
     */
    template<typename U>
    [[nodiscard]] auto operator<(const Field<T, U>& other) const {
        return BinaryExpr<BinaryOp::Less, field_expr_type, FieldExpr<T, U>>{
            expr_, other.expr()
        };
    }

    /**
     * @brief Less-than-or-equal comparison: field <= value
     */
    template<typename U>
    [[nodiscard]] auto operator<=(const U& value) const {
        return BinaryExpr<BinaryOp::LessEqual, field_expr_type, ConstExpr<U>>{
            expr_, ConstExpr<U>{value}
        };
    }

    /**
     * @brief Field-to-field less-than-or-equal: field <= other_field
     */
    template<typename U>
    [[nodiscard]] auto operator<=(const Field<T, U>& other) const {
        return BinaryExpr<BinaryOp::LessEqual, field_expr_type, FieldExpr<T, U>>{
            expr_, other.expr()
        };
    }

    /**
     * @brief Greater-than comparison: field > value
     */
    template<typename U>
    [[nodiscard]] auto operator>(const U& value) const {
        return BinaryExpr<BinaryOp::Greater, field_expr_type, ConstExpr<U>>{
            expr_, ConstExpr<U>{value}
        };
    }

    /**
     * @brief Field-to-field greater-than: field > other_field
     */
    template<typename U>
    [[nodiscard]] auto operator>(const Field<T, U>& other) const {
        return BinaryExpr<BinaryOp::Greater, field_expr_type, FieldExpr<T, U>>{
            expr_, other.expr()
        };
    }

    /**
     * @brief Greater-than-or-equal comparison: field >= value
     */
    template<typename U>
    [[nodiscard]] auto operator>=(const U& value) const {
        return BinaryExpr<BinaryOp::GreaterEqual, field_expr_type, ConstExpr<U>>{
            expr_, ConstExpr<U>{value}
        };
    }

    /**
     * @brief Field-to-field greater-than-or-equal: field >= other_field
     */
    template<typename U>
    [[nodiscard]] auto operator>=(const Field<T, U>& other) const {
        return BinaryExpr<BinaryOp::GreaterEqual, field_expr_type, FieldExpr<T, U>>{
            expr_, other.expr()
        };
    }

private:
    field_expr_type expr_;
};

// Deduction guides
template<typename T, typename FieldType>
Field(std::string, FieldType (T::*)() const) -> Field<T, FieldType>;

template<typename T, typename FieldType>
Field(std::string, std::function<FieldType(const T&)>) -> Field<T, FieldType>;

/**
 * @brief Logical AND operator: expr1 && expr2
 */
template<typename Left, typename Right>
requires Expression<Left> && Expression<Right>
[[nodiscard]] auto operator&&(const Expr<Left>& left, const Expr<Right>& right) {
    return LogicalExpr<LogicalOp::And, Left, Right>{
        left.derived(), right.derived()
    };
}

/**
 * @brief Logical OR operator: expr1 || expr2
 */
template<typename Left, typename Right>
requires Expression<Left> && Expression<Right>
[[nodiscard]] auto operator||(const Expr<Left>& left, const Expr<Right>& right) {
    return LogicalExpr<LogicalOp::Or, Left, Right>{
        left.derived(), right.derived()
    };
}

} // namespace learnql::query

#endif // LEARNQL_QUERY_FIELD_HPP
