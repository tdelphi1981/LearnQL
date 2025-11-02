#ifndef LEARNQL_QUERY_GROUP_BY_HPP
#define LEARNQL_QUERY_GROUP_BY_HPP

#include <unordered_map>
#include <vector>
#include <functional>
#include <numeric>
#include <algorithm>
#include <ranges>

namespace learnql::query {

/**
 * @brief Result of a group-by operation
 * @tparam GroupKey Type of the grouping key
 * @tparam AggValue Type of the aggregated value
 */
template<typename GroupKey, typename AggValue>
struct GroupByResult {
    GroupKey key;
    AggValue value;
    std::size_t count;
};

/**
 * @brief Group-by operation helper
 * @tparam T Record type
 * @tparam GroupKey Type of the grouping key
 *
 * Provides SQL-like GROUP BY functionality with aggregations.
 *
 * Example:
 * @code
 * auto results = GroupBy::group_by(
 *     students,
 *     [](const auto& s) { return s.department; },  // Group key
 *     [](const auto& group) {                       // Aggregate
 *         return std::ranges::fold_left(
 *             group | std::views::transform([](auto& s) { return s.gpa; }),
 *             0.0, std::plus{}
 *         ) / group.size();
 *     }
 * );
 * @endcode
 */
template<typename T, typename GroupKey>
class GroupBy {
public:
    using key_extractor_type = std::function<GroupKey(const T&)>;
    using group_map_type = std::unordered_map<GroupKey, std::vector<T>>;

    /**
     * @brief Performs a group-by operation with custom aggregation
     * @tparam Container Container type (e.g., vector)
     * @tparam AggFunc Aggregation function type
     * @param records Records to group
     * @param key_extractor Function to extract group key
     * @param agg_func Aggregation function
     * @return Vector of group-by results
     */
    template<typename Container, typename AggFunc>
    static auto group_by(
        const Container& records,
        key_extractor_type key_extractor,
        AggFunc agg_func)
    {
        using agg_value_type = std::invoke_result_t<AggFunc, const std::vector<T>&>;

        // Group records by key
        group_map_type groups;
        for (const auto& record : records) {
            auto key = key_extractor(record);
            groups[key].push_back(record);
        }

        // Apply aggregation to each group
        std::vector<GroupByResult<GroupKey, agg_value_type>> results;
        results.reserve(groups.size());

        for (auto& [key, group] : groups) {
            results.push_back({
                key,
                agg_func(group),
                group.size()
            });
        }

        return results;
    }

    /**
     * @brief Groups records and counts them
     * @tparam Container Container type
     * @param records Records to group
     * @param key_extractor Function to extract group key
     * @return Vector of group counts
     */
    template<typename Container>
    static auto count_by(
        const Container& records,
        key_extractor_type key_extractor)
    {
        return group_by(
            records,
            key_extractor,
            [](const std::vector<T>& group) { return group.size(); }
        );
    }

    /**
     * @brief Groups records and sums a field
     * @tparam Container Container type
     * @tparam KeyExtractor Key extractor callable type
     * @tparam ValueExtractor Value extractor callable type
     * @param records Records to group
     * @param key_extractor Function to extract group key
     * @param value_extractor Function to extract value to sum
     * @return Vector of group sums
     */
    template<typename Container, typename KeyExtractor, typename ValueExtractor>
    static auto sum_by(
        const Container& records,
        KeyExtractor&& key_extractor,
        ValueExtractor&& value_extractor)
    {
        return group_by(
            records,
            std::forward<KeyExtractor>(key_extractor),
            [value_extractor = std::forward<ValueExtractor>(value_extractor)](const std::vector<T>& group) {
                using FieldType = std::invoke_result_t<ValueExtractor, const T&>;
                FieldType sum{};
                for (const auto& record : group) {
                    sum += value_extractor(record);
                }
                return sum;
            }
        );
    }

    /**
     * @brief Groups records and computes average of a field
     * @tparam Container Container type
     * @tparam KeyExtractor Key extractor callable type
     * @tparam ValueExtractor Value extractor callable type
     * @param records Records to group
     * @param key_extractor Function to extract group key
     * @param value_extractor Function to extract value to average
     * @return Vector of group averages
     */
    template<typename Container, typename KeyExtractor, typename ValueExtractor>
    static auto average_by(
        const Container& records,
        KeyExtractor&& key_extractor,
        ValueExtractor&& value_extractor)
    {
        return group_by(
            records,
            std::forward<KeyExtractor>(key_extractor),
            [value_extractor = std::forward<ValueExtractor>(value_extractor)](const std::vector<T>& group) -> double {
                if (group.empty()) return 0.0;

                double sum = 0.0;
                for (const auto& record : group) {
                    sum += static_cast<double>(value_extractor(record));
                }
                return sum / group.size();
            }
        );
    }

    /**
     * @brief Groups records and finds minimum value of a field
     * @tparam Container Container type
     * @tparam KeyExtractor Key extractor callable type
     * @tparam ValueExtractor Value extractor callable type
     * @param records Records to group
     * @param key_extractor Function to extract group key
     * @param value_extractor Function to extract value
     * @return Vector of group minimums
     */
    template<typename Container, typename KeyExtractor, typename ValueExtractor>
    static auto min_by(
        const Container& records,
        KeyExtractor&& key_extractor,
        ValueExtractor&& value_extractor)
    {
        return group_by(
            records,
            std::forward<KeyExtractor>(key_extractor),
            [value_extractor = std::forward<ValueExtractor>(value_extractor)](const std::vector<T>& group) {
                using FieldType = std::invoke_result_t<ValueExtractor, const T&>;
                if (group.empty()) return FieldType{};

                auto min_val = value_extractor(group[0]);
                for (std::size_t i = 1; i < group.size(); ++i) {
                    auto val = value_extractor(group[i]);
                    if (val < min_val) {
                        min_val = val;
                    }
                }
                return min_val;
            }
        );
    }

    /**
     * @brief Groups records and finds maximum value of a field
     * @tparam Container Container type
     * @tparam KeyExtractor Key extractor callable type
     * @tparam ValueExtractor Value extractor callable type
     * @param records Records to group
     * @param key_extractor Function to extract group key
     * @param value_extractor Function to extract value
     * @return Vector of group maximums
     */
    template<typename Container, typename KeyExtractor, typename ValueExtractor>
    static auto max_by(
        const Container& records,
        KeyExtractor&& key_extractor,
        ValueExtractor&& value_extractor)
    {
        return group_by(
            records,
            std::forward<KeyExtractor>(key_extractor),
            [value_extractor = std::forward<ValueExtractor>(value_extractor)](const std::vector<T>& group) {
                using FieldType = std::invoke_result_t<ValueExtractor, const T&>;
                if (group.empty()) return FieldType{};

                auto max_val = value_extractor(group[0]);
                for (std::size_t i = 1; i < group.size(); ++i) {
                    auto val = value_extractor(group[i]);
                    if (val > max_val) {
                        max_val = val;
                    }
                }
                return max_val;
            }
        );
    }
};

/**
 * @brief Helper function to create a group-by operation
 * @tparam T Record type
 * @tparam GroupKey Group key type
 * @tparam Container Container type
 * @tparam AggFunc Aggregation function type
 * @param records Records to group
 * @param key_extractor Function to extract group key
 * @param agg_func Aggregation function
 * @return Vector of group-by results
 */
template<typename T, typename GroupKey, typename Container, typename AggFunc>
auto group_by(
    const Container& records,
    std::function<GroupKey(const T&)> key_extractor,
    AggFunc agg_func)
{
    return GroupBy<T, GroupKey>::group_by(records, key_extractor, agg_func);
}

} // namespace learnql::query

#endif // LEARNQL_QUERY_GROUP_BY_HPP
