#ifndef LEARNQL_QUERY_EXPRESSIONS_CONST_EXPR_HPP
#define LEARNQL_QUERY_EXPRESSIONS_CONST_EXPR_HPP

#include "Expr.hpp"
#include <sstream>

namespace learnql::query::expressions {

/**
 * @brief Expression representing a constant value
 * @tparam T Type of the constant value
 *
 * Example:
 * @code
 * ConstExpr<int> age_limit{18};
 * ConstExpr<std::string> name{"Alice"};
 * @endcode
 */
template<typename T>
class ConstExpr : public Expr<ConstExpr<T>> {
public:
    using value_type = T;

    /**
     * @brief Constructs a constant expression
     * @param value The constant value
     */
    explicit constexpr ConstExpr(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
        : value_(std::move(value)) {}

    /**
     * @brief Evaluates to the constant value (ignores object)
     * @tparam U Object type (unused)
     * @return The constant value
     */
    template<typename U>
    [[nodiscard]] constexpr const T& evaluate(const U&) const noexcept {
        return value_;
    }

    /**
     * @brief Gets the constant value directly
     */
    [[nodiscard]] constexpr const T& value() const noexcept {
        return value_;
    }

    /**
     * @brief Converts to string representation
     */
    [[nodiscard]] std::string to_string() const {
        std::ostringstream oss;
        if constexpr (std::is_same_v<T, std::string>) {
            oss << "\"" << value_ << "\"";
        } else if constexpr (std::is_same_v<T, bool>) {
            oss << (value_ ? "true" : "false");
        } else {
            oss << value_;
        }
        return oss.str();
    }

private:
    T value_;
};

// Deduction guide
template<typename T>
ConstExpr(T) -> ConstExpr<T>;

} // namespace learnql::query::expressions

#endif // LEARNQL_QUERY_EXPRESSIONS_CONST_EXPR_HPP
