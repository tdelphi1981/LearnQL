#ifndef LEARNQL_QUERY_EXPRESSIONS_BINARY_EXPR_HPP
#define LEARNQL_QUERY_EXPRESSIONS_BINARY_EXPR_HPP

#include "Expr.hpp"
#include "ConstExpr.hpp"
#include "FieldExpr.hpp"
#include <sstream>

namespace learnql::query::expressions {

/**
 * @brief Expression representing a binary comparison operation
 * @tparam Op Binary operator type
 * @tparam Left Left expression type
 * @tparam Right Right expression type
 *
 * Example:
 * @code
 * auto expr = BinaryExpr<BinaryOp::Greater, FieldExpr<...>, ConstExpr<int>>{...};
 * // Represents: field > 18
 * @endcode
 */
template<BinaryOp Op, typename Left, typename Right>
class BinaryExpr : public Expr<BinaryExpr<Op, Left, Right>> {
public:
    /**
     * @brief Constructs a binary expression
     * @param left Left operand
     * @param right Right operand
     */
    BinaryExpr(Left left, Right right)
        : left_(std::move(left)),
          right_(std::move(right)) {}

    /**
     * @brief Evaluates the binary expression
     * @tparam T Object type
     * @param obj Object to evaluate against
     * @return Result of comparison
     */
    template<typename T>
    [[nodiscard]] bool evaluate(const T& obj) const {
        auto left_val = left_.evaluate(obj);
        auto right_val = right_.evaluate(obj);

        if constexpr (Op == BinaryOp::Equal) {
            return left_val == right_val;
        } else if constexpr (Op == BinaryOp::NotEqual) {
            return left_val != right_val;
        } else if constexpr (Op == BinaryOp::Less) {
            return left_val < right_val;
        } else if constexpr (Op == BinaryOp::LessEqual) {
            return left_val <= right_val;
        } else if constexpr (Op == BinaryOp::Greater) {
            return left_val > right_val;
        } else if constexpr (Op == BinaryOp::GreaterEqual) {
            return left_val >= right_val;
        } else {
            static_assert(Op == BinaryOp::Equal, "Unknown binary operator");
            return false;
        }
    }

    /**
     * @brief Converts to string representation
     */
    [[nodiscard]] std::string to_string() const {
        std::ostringstream oss;
        oss << "(" << left_.to_string() << " "
            << binary_op_to_string(Op) << " "
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

#endif // LEARNQL_QUERY_EXPRESSIONS_BINARY_EXPR_HPP
