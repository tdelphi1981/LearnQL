#ifndef LEARNQL_QUERY_EXPRESSIONS_EXPR_HPP
#define LEARNQL_QUERY_EXPRESSIONS_EXPR_HPP

#include <type_traits>
#include <functional>
#include <string>

namespace learnql::query::expressions {

/**
 * @brief Base class for all expression types (CRTP pattern)
 * @tparam Derived The derived expression type
 *
 * Expression templates allow us to build complex expressions at compile-time
 * without evaluating them until necessary.
 *
 * Example:
 * @code
 * auto expr = (age > 18) && (name == "Alice");
 * // expr is an expression tree, not yet evaluated
 * @endcode
 */
template<typename Derived>
class Expr {
public:
    /**
     * @brief CRTP accessor to derived class
     */
    [[nodiscard]] const Derived& derived() const noexcept {
        return static_cast<const Derived&>(*this);
    }

    /**
     * @brief Evaluates the expression for a given object
     * @tparam T Object type
     * @param obj Object to evaluate against
     * @return Result of evaluation
     */
    template<typename T>
    [[nodiscard]] auto evaluate(const T& obj) const {
        return derived().evaluate(obj);
    }

    /**
     * @brief Converts expression to a string (for debugging)
     */
    [[nodiscard]] std::string to_string() const {
        return derived().to_string();
    }
};

/**
 * @brief Binary operator types
 */
enum class BinaryOp {
    Equal,              // ==
    NotEqual,           // !=
    Less,               // <
    LessEqual,          // <=
    Greater,            // >
    GreaterEqual        // >=
};

/**
 * @brief Converts BinaryOp to string
 */
[[nodiscard]] inline constexpr const char* binary_op_to_string(BinaryOp op) noexcept {
    switch (op) {
        case BinaryOp::Equal: return "==";
        case BinaryOp::NotEqual: return "!=";
        case BinaryOp::Less: return "<";
        case BinaryOp::LessEqual: return "<=";
        case BinaryOp::Greater: return ">";
        case BinaryOp::GreaterEqual: return ">=";
        default: return "?";
    }
}

/**
 * @brief Logical operator types
 */
enum class LogicalOp {
    And,    // &&
    Or      // ||
};

/**
 * @brief Converts LogicalOp to string
 */
[[nodiscard]] inline constexpr const char* logical_op_to_string(LogicalOp op) noexcept {
    switch (op) {
        case LogicalOp::And: return "&&";
        case LogicalOp::Or: return "||";
        default: return "?";
    }
}

/**
 * @brief Concept for expression types
 */
template<typename T>
concept Expression = requires(const T& expr) {
    { expr.to_string() } -> std::convertible_to<std::string>;
};

} // namespace learnql::query::expressions

#endif // LEARNQL_QUERY_EXPRESSIONS_EXPR_HPP
