#ifndef LEARNQL_QUERY_EXPRESSIONS_LOGICAL_EXPR_HPP
#define LEARNQL_QUERY_EXPRESSIONS_LOGICAL_EXPR_HPP

#include "Expr.hpp"
#include <sstream>

namespace learnql::query::expressions {

/**
 * @brief Expression representing a logical operation (AND/OR)
 * @tparam Op Logical operator type
 * @tparam Left Left expression type
 * @tparam Right Right expression type
 *
 * Example:
 * @code
 * auto expr = LogicalExpr<LogicalOp::And, BinaryExpr<...>, BinaryExpr<...>>{...};
 * // Represents: (age > 18) && (name == "Alice")
 * @endcode
 */
template<LogicalOp Op, typename Left, typename Right>
class LogicalExpr : public Expr<LogicalExpr<Op, Left, Right>> {
public:
    /**
     * @brief Constructs a logical expression
     * @param left Left operand
     * @param right Right operand
     */
    LogicalExpr(Left left, Right right)
        : left_(std::move(left)),
          right_(std::move(right)) {}

    /**
     * @brief Evaluates the logical expression
     * @tparam T Object type
     * @param obj Object to evaluate against
     * @return Result of logical operation
     */
    template<typename T>
    [[nodiscard]] bool evaluate(const T& obj) const {
        if constexpr (Op == LogicalOp::And) {
            // Short-circuit evaluation
            return left_.evaluate(obj) && right_.evaluate(obj);
        } else if constexpr (Op == LogicalOp::Or) {
            // Short-circuit evaluation
            return left_.evaluate(obj) || right_.evaluate(obj);
        } else {
            static_assert(Op == LogicalOp::And, "Unknown logical operator");
            return false;
        }
    }

    /**
     * @brief Converts to string representation
     */
    [[nodiscard]] std::string to_string() const {
        std::ostringstream oss;
        oss << "(" << left_.to_string() << " "
            << logical_op_to_string(Op) << " "
            << right_.to_string() << ")";
        return oss.str();
    }

    /**
     * @brief Gets the left operand
     */
    [[nodiscard]] const Left& left() const noexcept {
        return left_;
    }

    /**
     * @brief Gets the right operand
     */
    [[nodiscard]] const Right& right() const noexcept {
        return right_;
    }

private:
    Left left_;
    Right right_;
};

} // namespace learnql::query::expressions

#endif // LEARNQL_QUERY_EXPRESSIONS_LOGICAL_EXPR_HPP
