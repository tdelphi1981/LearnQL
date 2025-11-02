#ifndef LEARNQL_RANGES_ADAPTORS_HPP
#define LEARNQL_RANGES_ADAPTORS_HPP

#include <ranges>
#include <algorithm>
#include <functional>
#include <concepts>
#include <numeric>

namespace learnql::ranges {

/**
 * @brief Custom range adaptor for ordering by a field
 * @tparam Proj Projection function type
 *
 * Example:
 * @code
 * auto result = students.view()
 *     | order_by(&Student::get_age)
 *     | std::views::take(10);
 * @endcode
 */
template<typename Proj>
class OrderByAdaptor {
public:
    explicit constexpr OrderByAdaptor(Proj proj, bool ascending = true)
        : proj_(std::move(proj)), ascending_(ascending) {}

    template<std::ranges::input_range R>
    [[nodiscard]] auto operator()(R&& range) const {
        auto vec = std::vector(std::ranges::begin(range), std::ranges::end(range));

        if (ascending_) {
            std::ranges::sort(vec, std::less{}, proj_);
        } else {
            std::ranges::sort(vec, std::greater{}, proj_);
        }

        return vec;
    }

private:
    Proj proj_;
    bool ascending_;
};

/**
 * @brief Helper function to create an order_by adaptor
 */
template<typename Proj>
[[nodiscard]] constexpr auto order_by(Proj&& proj, bool ascending = true) {
    return OrderByAdaptor<std::decay_t<Proj>>{std::forward<Proj>(proj), ascending};
}

/**
 * @brief Pipe operator for order_by
 */
template<std::ranges::input_range R, typename Proj>
[[nodiscard]] auto operator|(R&& range, const OrderByAdaptor<Proj>& adaptor) {
    return adaptor(std::forward<R>(range));
}

/**
 * @brief Custom range adaptor for selecting specific fields
 * @tparam Proj Projection function type
 *
 * Example:
 * @code
 * auto names = students.view()
 *     | select(&Student::get_name);
 * @endcode
 */
template<typename Proj>
class SelectAdaptor {
public:
    explicit constexpr SelectAdaptor(Proj proj)
        : proj_(std::move(proj)) {}

    template<std::ranges::input_range R>
    [[nodiscard]] auto operator()(R&& range) const {
        return std::forward<R>(range) | std::views::transform(proj_);
    }

private:
    Proj proj_;
};

/**
 * @brief Helper function to create a select adaptor
 */
template<typename Proj>
[[nodiscard]] constexpr auto select(Proj&& proj) {
    return SelectAdaptor<std::decay_t<Proj>>{std::forward<Proj>(proj)};
}

/**
 * @brief Pipe operator for select
 */
template<std::ranges::input_range R, typename Proj>
[[nodiscard]] auto operator|(R&& range, const SelectAdaptor<Proj>& adaptor) {
    return adaptor(std::forward<R>(range));
}

/**
 * @brief Custom range adaptor for distinct values
 *
 * Example:
 * @code
 * auto unique_ages = students.view()
 *     | select(&Student::get_age)
 *     | distinct();
 * @endcode
 */
class DistinctAdaptor {
public:
    template<std::ranges::input_range R>
    [[nodiscard]] auto operator()(R&& range) const {
        auto vec = std::vector(std::ranges::begin(range), std::ranges::end(range));

        std::ranges::sort(vec);
        auto [first, last] = std::ranges::unique(vec);
        vec.erase(first, last);

        return vec;
    }
};

/**
 * @brief Helper function to create a distinct adaptor
 */
[[nodiscard]] constexpr auto distinct() {
    return DistinctAdaptor{};
}

/**
 * @brief Pipe operator for distinct
 */
template<std::ranges::input_range R>
[[nodiscard]] auto operator|(R&& range, const DistinctAdaptor& adaptor) {
    return adaptor(std::forward<R>(range));
}

/**
 * @brief Custom range adaptor for limiting results
 *
 * This is similar to SQL's LIMIT clause.
 *
 * Example:
 * @code
 * auto top10 = students.view()
 *     | order_by(&Student::get_gpa, false)
 *     | limit(10);
 * @endcode
 */
class LimitAdaptor {
public:
    explicit constexpr LimitAdaptor(std::size_t n)
        : n_(n) {}

    template<std::ranges::input_range R>
    [[nodiscard]] auto operator()(R&& range) const {
        return std::forward<R>(range) | std::views::take(n_);
    }

private:
    std::size_t n_;
};

/**
 * @brief Helper function to create a limit adaptor
 */
[[nodiscard]] constexpr auto limit(std::size_t n) {
    return LimitAdaptor{n};
}

/**
 * @brief Pipe operator for limit
 */
template<std::ranges::input_range R>
[[nodiscard]] auto operator|(R&& range, const LimitAdaptor& adaptor) {
    return adaptor(std::forward<R>(range));
}

/**
 * @brief Custom range adaptor for skipping results
 *
 * This is similar to SQL's OFFSET clause.
 *
 * Example:
 * @code
 * auto page2 = students.view()
 *     | skip(10)
 *     | limit(10);
 * @endcode
 */
class SkipAdaptor {
public:
    explicit constexpr SkipAdaptor(std::size_t n)
        : n_(n) {}

    template<std::ranges::input_range R>
    [[nodiscard]] auto operator()(R&& range) const {
        return std::forward<R>(range) | std::views::drop(n_);
    }

private:
    std::size_t n_;
};

/**
 * @brief Helper function to create a skip adaptor
 */
[[nodiscard]] constexpr auto skip(std::size_t n) {
    return SkipAdaptor{n};
}

/**
 * @brief Pipe operator for skip
 */
template<std::ranges::input_range R>
[[nodiscard]] auto operator|(R&& range, const SkipAdaptor& adaptor) {
    return adaptor(std::forward<R>(range));
}

/**
 * @brief Aggregation functions for ranges
 */
namespace aggregates {

/**
 * @brief Counts elements in a range
 */
template<std::ranges::input_range R>
[[nodiscard]] auto count(R&& range) {
    return std::ranges::distance(range);
}

/**
 * @brief Sums elements in a range
 */
template<std::ranges::input_range R>
[[nodiscard]] auto sum(R&& range) {
    using value_type = std::ranges::range_value_t<R>;
    return std::accumulate(std::ranges::begin(range), std::ranges::end(range), value_type{});
}

/**
 * @brief Computes average of elements in a range
 */
template<std::ranges::input_range R>
[[nodiscard]] auto average(R&& range) {
    auto sum_val = sum(range);
    auto count_val = count(range);
    return count_val > 0 ? sum_val / static_cast<double>(count_val) : 0.0;
}

/**
 * @brief Finds minimum element in a range
 */
template<std::ranges::input_range R>
[[nodiscard]] auto min(R&& range) {
    return *std::ranges::min_element(range);
}

/**
 * @brief Finds maximum element in a range
 */
template<std::ranges::input_range R>
[[nodiscard]] auto max(R&& range) {
    return *std::ranges::max_element(range);
}

} // namespace aggregates

} // namespace learnql::ranges

#endif // LEARNQL_RANGES_ADAPTORS_HPP
