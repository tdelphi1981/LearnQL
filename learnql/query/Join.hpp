#ifndef LEARNQL_QUERY_JOIN_HPP
#define LEARNQL_QUERY_JOIN_HPP

#include <vector>
#include <functional>
#include <unordered_map>
#include <optional>

namespace learnql::query {

/**
 * @brief Result of a join operation
 * @tparam Left Left table record type
 * @tparam Right Right table record type
 */
template<typename Left, typename Right>
struct JoinResult {
    Left left;
    std::optional<Right> right; // Optional for left/right outer joins

    /**
     * @brief Checks if this is an inner join result (both sides present)
     */
    [[nodiscard]] bool is_inner() const noexcept {
        return right.has_value();
    }
};

/**
 * @brief Join types
 */
enum class JoinType {
    Inner,      // Only matching records
    LeftOuter,  // All left records, matching right records (nulls if no match)
    RightOuter, // All right records, matching left records (nulls if no match)
    FullOuter   // All records from both sides
};

/**
 * @brief Join operations for combining data from multiple tables
 * @tparam Left Left table record type
 * @tparam Right Right table record type
 *
 * Provides SQL-like join operations.
 *
 * Example:
 * @code
 * auto results = Join::inner_join(
 *     students,
 *     enrollments,
 *     [](const auto& s) { return s.id; },          // Left key
 *     [](const auto& e) { return e.student_id; }   // Right key
 * );
 * @endcode
 */
template<typename Left, typename Right>
class Join {
public:
    /**
     * @brief Performs an inner join
     * @tparam LeftContainer Left container type
     * @tparam RightContainer Right container type
     * @tparam LeftKeyExtractor Left key extractor callable type
     * @tparam RightKeyExtractor Right key extractor callable type
     * @param left_records Left table records
     * @param right_records Right table records
     * @param left_key_extractor Function to extract key from left record
     * @param right_key_extractor Function to extract key from right record
     * @return Vector of join results (only matching records)
     */
    template<typename LeftContainer, typename RightContainer,
             typename LeftKeyExtractor, typename RightKeyExtractor>
    static auto inner_join(
        const LeftContainer& left_records,
        const RightContainer& right_records,
        LeftKeyExtractor&& left_key_extractor,
        RightKeyExtractor&& right_key_extractor)
    {
        using LeftKey = std::invoke_result_t<LeftKeyExtractor, const Left&>;
        using RightKey = std::invoke_result_t<RightKeyExtractor, const Right&>;

        static_assert(std::is_same_v<LeftKey, RightKey>,
                     "Join keys must have the same type");

        using join_result_type = JoinResult<Left, Right>;
        std::vector<join_result_type> results;

        // Build hash map for right table
        std::unordered_multimap<LeftKey, Right> right_map;
        for (const auto& right : right_records) {
            right_map.emplace(right_key_extractor(right), right);
        }

        // Join with left table
        for (const auto& left : left_records) {
            auto left_key = left_key_extractor(left);
            auto range = right_map.equal_range(left_key);

            for (auto it = range.first; it != range.second; ++it) {
                results.push_back({left, it->second});
            }
        }

        return results;
    }

    /**
     * @brief Performs a left outer join
     * @tparam LeftContainer Left container type
     * @tparam RightContainer Right container type
     * @tparam LeftKeyExtractor Left key extractor callable type
     * @tparam RightKeyExtractor Right key extractor callable type
     * @param left_records Left table records
     * @param right_records Right table records
     * @param left_key_extractor Function to extract key from left record
     * @param right_key_extractor Function to extract key from right record
     * @return Vector of join results (all left records, matching right records)
     */
    template<typename LeftContainer, typename RightContainer,
             typename LeftKeyExtractor, typename RightKeyExtractor>
    static auto left_join(
        const LeftContainer& left_records,
        const RightContainer& right_records,
        LeftKeyExtractor&& left_key_extractor,
        RightKeyExtractor&& right_key_extractor)
    {
        using LeftKey = std::invoke_result_t<LeftKeyExtractor, const Left&>;
        using RightKey = std::invoke_result_t<RightKeyExtractor, const Right&>;

        static_assert(std::is_same_v<LeftKey, RightKey>,
                     "Join keys must have the same type");

        using join_result_type = JoinResult<Left, Right>;
        std::vector<join_result_type> results;

        // Build hash map for right table
        std::unordered_multimap<LeftKey, Right> right_map;
        for (const auto& right : right_records) {
            right_map.emplace(right_key_extractor(right), right);
        }

        // Join with left table
        for (const auto& left : left_records) {
            auto left_key = left_key_extractor(left);
            auto range = right_map.equal_range(left_key);

            if (range.first == range.second) {
                // No match - include left with nullopt right
                results.push_back({left, std::nullopt});
            } else {
                // One or more matches
                for (auto it = range.first; it != range.second; ++it) {
                    results.push_back({left, it->second});
                }
            }
        }

        return results;
    }

    /**
     * @brief Performs a cross join (Cartesian product)
     * @tparam LeftContainer Left container type
     * @tparam RightContainer Right container type
     * @param left_records Left table records
     * @param right_records Right table records
     * @return Vector of all combinations
     */
    template<typename LeftContainer, typename RightContainer>
    static auto cross_join(
        const LeftContainer& left_records,
        const RightContainer& right_records)
    {
        using join_result_type = JoinResult<Left, Right>;
        std::vector<join_result_type> results;

        for (const auto& left : left_records) {
            for (const auto& right : right_records) {
                results.push_back({left, right});
            }
        }

        return results;
    }

    /**
     * @brief Performs a semi join
     * @tparam LeftContainer Left container type
     * @tparam RightContainer Right container type
     * @tparam LeftKeyExtractor Left key extractor callable type
     * @tparam RightKeyExtractor Right key extractor callable type
     * @param left_records Left table records
     * @param right_records Right table records
     * @param left_key_extractor Function to extract key from left record
     * @param right_key_extractor Function to extract key from right record
     * @return Vector of left records that have a match in right table
     */
    template<typename LeftContainer, typename RightContainer,
             typename LeftKeyExtractor, typename RightKeyExtractor>
    static auto semi_join(
        const LeftContainer& left_records,
        const RightContainer& right_records,
        LeftKeyExtractor&& left_key_extractor,
        RightKeyExtractor&& right_key_extractor)
    {
        using LeftKey = std::invoke_result_t<LeftKeyExtractor, const Left&>;
        using RightKey = std::invoke_result_t<RightKeyExtractor, const Right&>;

        static_assert(std::is_same_v<LeftKey, RightKey>,
                     "Join keys must have the same type");

        std::vector<Left> results;

        // Build set of right keys
        std::unordered_set<LeftKey> right_keys;
        for (const auto& right : right_records) {
            right_keys.insert(right_key_extractor(right));
        }

        // Filter left records
        for (const auto& left : left_records) {
            if (right_keys.contains(left_key_extractor(left))) {
                results.push_back(left);
            }
        }

        return results;
    }

    /**
     * @brief Performs an anti join
     * @tparam LeftContainer Left container type
     * @tparam RightContainer Right container type
     * @tparam LeftKeyExtractor Left key extractor callable type
     * @tparam RightKeyExtractor Right key extractor callable type
     * @param left_records Left table records
     * @param right_records Right table records
     * @param left_key_extractor Function to extract key from left record
     * @param right_key_extractor Function to extract key from right record
     * @return Vector of left records that DON'T have a match in right table
     */
    template<typename LeftContainer, typename RightContainer,
             typename LeftKeyExtractor, typename RightKeyExtractor>
    static auto anti_join(
        const LeftContainer& left_records,
        const RightContainer& right_records,
        LeftKeyExtractor&& left_key_extractor,
        RightKeyExtractor&& right_key_extractor)
    {
        using LeftKey = std::invoke_result_t<LeftKeyExtractor, const Left&>;
        using RightKey = std::invoke_result_t<RightKeyExtractor, const Right&>;

        static_assert(std::is_same_v<LeftKey, RightKey>,
                     "Join keys must have the same type");

        std::vector<Left> results;

        // Build set of right keys
        std::unordered_set<LeftKey> right_keys;
        for (const auto& right : right_records) {
            right_keys.insert(right_key_extractor(right));
        }

        // Filter left records
        for (const auto& left : left_records) {
            if (!right_keys.contains(left_key_extractor(left))) {
                results.push_back(left);
            }
        }

        return results;
    }
};

/**
 * @brief Helper function for inner join
 */
template<typename Left, typename Right, typename LeftContainer, typename RightContainer,
         typename LeftKey, typename RightKey>
auto inner_join(
    const LeftContainer& left_records,
    const RightContainer& right_records,
    std::function<LeftKey(const Left&)> left_key_extractor,
    std::function<RightKey(const Right&)> right_key_extractor)
{
    return Join<Left, Right>::inner_join(
        left_records, right_records,
        left_key_extractor, right_key_extractor
    );
}

/**
 * @brief Helper function for left join
 */
template<typename Left, typename Right, typename LeftContainer, typename RightContainer,
         typename LeftKey, typename RightKey>
auto left_join(
    const LeftContainer& left_records,
    const RightContainer& right_records,
    std::function<LeftKey(const Left&)> left_key_extractor,
    std::function<RightKey(const Right&)> right_key_extractor)
{
    return Join<Left, Right>::left_join(
        left_records, right_records,
        left_key_extractor, right_key_extractor
    );
}

} // namespace learnql::query

#endif // LEARNQL_QUERY_JOIN_HPP
