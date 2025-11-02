#ifndef LEARNQL_DEBUG_STATISTICS_HPP
#define LEARNQL_DEBUG_STATISTICS_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace learnql::debug {

/**
 * @brief Statistics for a table
 */
struct TableStatistics {
    std::string table_name;
    std::size_t row_count = 0;
    std::size_t page_count = 0;
    std::size_t total_bytes = 0;
    std::size_t index_count = 0;

    [[nodiscard]] double avg_row_size() const {
        return row_count > 0 ? static_cast<double>(total_bytes) / row_count : 0.0;
    }

    [[nodiscard]] double avg_rows_per_page() const {
        return page_count > 0 ? static_cast<double>(row_count) / page_count : 0.0;
    }

    void print() const {
        std::cout << "Table: " << table_name << "\n";
        std::cout << "  Rows: " << row_count << "\n";
        std::cout << "  Pages: " << page_count << "\n";
        std::cout << "  Total size: " << total_bytes << " bytes\n";
        std::cout << "  Avg row size: " << std::fixed << std::setprecision(2)
                  << avg_row_size() << " bytes\n";
        std::cout << "  Avg rows per page: " << std::fixed << std::setprecision(2)
                  << avg_rows_per_page() << "\n";
        std::cout << "  Indexes: " << index_count << "\n";
    }
};

/**
 * @brief Statistics for an index
 */
struct IndexStatistics {
    std::string index_name;
    std::string table_name;
    std::string indexed_field;
    std::size_t entry_count = 0;
    std::size_t unique_values = 0;
    std::size_t total_bytes = 0;

    [[nodiscard]] double selectivity() const {
        return entry_count > 0 ? static_cast<double>(unique_values) / entry_count : 0.0;
    }

    void print() const {
        std::cout << "Index: " << index_name << "\n";
        std::cout << "  Table: " << table_name << "\n";
        std::cout << "  Field: " << indexed_field << "\n";
        std::cout << "  Entries: " << entry_count << "\n";
        std::cout << "  Unique values: " << unique_values << "\n";
        std::cout << "  Selectivity: " << std::fixed << std::setprecision(4)
                  << selectivity() << "\n";
        std::cout << "  Total size: " << total_bytes << " bytes\n";
    }
};

/**
 * @brief Query statistics
 */
struct QueryStatistics {
    std::string query_description;
    std::size_t execution_count = 0;
    std::chrono::microseconds total_duration = std::chrono::microseconds::zero();
    std::size_t total_rows_returned = 0;

    [[nodiscard]] double avg_duration_ms() const {
        return execution_count > 0 ? (total_duration.count() / (1000.0 * execution_count)) : 0.0;
    }

    [[nodiscard]] double avg_rows_returned() const {
        return execution_count > 0 ? static_cast<double>(total_rows_returned) / execution_count : 0.0;
    }

    void print() const {
        std::cout << "Query: " << query_description << "\n";
        std::cout << "  Executions: " << execution_count << "\n";
        std::cout << "  Avg duration: " << std::fixed << std::setprecision(3)
                  << avg_duration_ms() << " ms\n";
        std::cout << "  Avg rows: " << std::fixed << std::setprecision(2)
                  << avg_rows_returned() << "\n";
    }
};

/**
 * @brief Statistics collector for database objects
 *
 * Collects and maintains statistics about tables, indexes, and queries
 * to help with query optimization and debugging.
 */
class StatisticsCollector {
public:
    /**
     * @brief Gets the singleton instance
     */
    static StatisticsCollector& instance() {
        static StatisticsCollector collector;
        return collector;
    }

    /**
     * @brief Records table statistics
     */
    void record_table(const TableStatistics& stats) {
        table_stats_[stats.table_name] = stats;
    }

    /**
     * @brief Records index statistics
     */
    void record_index(const IndexStatistics& stats) {
        index_stats_[stats.index_name] = stats;
    }

    /**
     * @brief Records a query execution
     */
    void record_query(const std::string& query_desc,
                     std::chrono::microseconds duration,
                     std::size_t rows_returned)
    {
        auto& stats = query_stats_[query_desc];
        stats.query_description = query_desc;
        stats.execution_count++;
        stats.total_duration += duration;
        stats.total_rows_returned += rows_returned;
    }

    /**
     * @brief Gets table statistics
     */
    [[nodiscard]] const TableStatistics* get_table_stats(const std::string& table_name) const {
        auto it = table_stats_.find(table_name);
        return it != table_stats_.end() ? &it->second : nullptr;
    }

    /**
     * @brief Gets index statistics
     */
    [[nodiscard]] const IndexStatistics* get_index_stats(const std::string& index_name) const {
        auto it = index_stats_.find(index_name);
        return it != index_stats_.end() ? &it->second : nullptr;
    }

    /**
     * @brief Prints all statistics
     */
    void print_all() const {
        std::cout << "\n=== Database Statistics ===\n\n";

        // Table statistics
        if (!table_stats_.empty()) {
            std::cout << "--- Table Statistics ---\n";
            for (const auto& [name, stats] : table_stats_) {
                stats.print();
                std::cout << "\n";
            }
        }

        // Index statistics
        if (!index_stats_.empty()) {
            std::cout << "--- Index Statistics ---\n";
            for (const auto& [name, stats] : index_stats_) {
                stats.print();
                std::cout << "\n";
            }
        }

        // Query statistics
        if (!query_stats_.empty()) {
            std::cout << "--- Query Statistics ---\n";

            // Sort by execution count (most frequently executed first)
            std::vector<QueryStatistics> sorted_queries;
            for (const auto& [desc, stats] : query_stats_) {
                sorted_queries.push_back(stats);
            }

            std::sort(sorted_queries.begin(), sorted_queries.end(),
                     [](const auto& a, const auto& b) {
                         return a.execution_count > b.execution_count;
                     });

            for (const auto& stats : sorted_queries) {
                stats.print();
                std::cout << "\n";
            }
        }
    }

    /**
     * @brief Clears all statistics
     */
    void clear() {
        table_stats_.clear();
        index_stats_.clear();
        query_stats_.clear();
    }

private:
    StatisticsCollector() = default;
    ~StatisticsCollector() = default;

    StatisticsCollector(const StatisticsCollector&) = delete;
    StatisticsCollector& operator=(const StatisticsCollector&) = delete;

    std::unordered_map<std::string, TableStatistics> table_stats_;
    std::unordered_map<std::string, IndexStatistics> index_stats_;
    std::unordered_map<std::string, QueryStatistics> query_stats_;
};

} // namespace learnql::debug

#endif // LEARNQL_DEBUG_STATISTICS_HPP
