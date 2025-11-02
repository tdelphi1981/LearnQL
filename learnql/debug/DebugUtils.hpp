#ifndef LEARNQL_DEBUG_DEBUG_UTILS_HPP
#define LEARNQL_DEBUG_DEBUG_UTILS_HPP

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <vector>

namespace learnql::debug {

/**
 * @brief Debug log levels
 */
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

/**
 * @brief Convert log level to string
 */
inline std::string log_level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error: return "ERROR";
    }
    return "UNKNOWN";
}

/**
 * @brief Simple logger for debugging
 */
class Logger {
public:
    /**
     * @brief Gets the singleton instance
     */
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    /**
     * @brief Sets the minimum log level
     */
    void set_log_level(LogLevel level) {
        min_level_ = level;
    }

    /**
     * @brief Logs a message
     */
    void log(LogLevel level, const std::string& message, const std::string& category = "") {
        if (level < min_level_) {
            return;
        }

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);

        std::cout << "[" << std::put_time(std::localtime(&time), "%H:%M:%S") << "] "
                  << "[" << log_level_to_string(level) << "] ";

        if (!category.empty()) {
            std::cout << "[" << category << "] ";
        }

        std::cout << message << "\n";
    }

    /**
     * @brief Debug log
     */
    void debug(const std::string& message, const std::string& category = "") {
        log(LogLevel::Debug, message, category);
    }

    /**
     * @brief Info log
     */
    void info(const std::string& message, const std::string& category = "") {
        log(LogLevel::Info, message, category);
    }

    /**
     * @brief Warning log
     */
    void warn(const std::string& message, const std::string& category = "") {
        log(LogLevel::Warning, message, category);
    }

    /**
     * @brief Error log
     */
    void error(const std::string& message, const std::string& category = "") {
        log(LogLevel::Error, message, category);
    }

private:
    Logger() : min_level_(LogLevel::Info) {}

    LogLevel min_level_;
};

/**
 * @brief Memory usage tracker
 */
class MemoryTracker {
public:
    /**
     * @brief Records an allocation
     */
    static void record_allocation(std::size_t bytes, const std::string& description = "") {
        instance().total_allocated_ += bytes;
        instance().current_usage_ += bytes;
        instance().peak_usage_ = std::max(instance().peak_usage_, instance().current_usage_);

        if (!description.empty()) {
            instance().allocations_[description] += bytes;
        }
    }

    /**
     * @brief Records a deallocation
     */
    static void record_deallocation(std::size_t bytes, const std::string& description = "") {
        instance().current_usage_ -= bytes;

        if (!description.empty() && instance().allocations_.count(description)) {
            instance().allocations_[description] -= bytes;
        }
    }

    /**
     * @brief Gets current memory usage
     */
    static std::size_t current_usage() {
        return instance().current_usage_;
    }

    /**
     * @brief Gets peak memory usage
     */
    static std::size_t peak_usage() {
        return instance().peak_usage_;
    }

    /**
     * @brief Prints memory usage summary
     */
    static void print_summary() {
        auto& tracker = instance();

        std::cout << "\n=== Memory Usage Summary ===\n";
        std::cout << "Current usage: " << format_bytes(tracker.current_usage_) << "\n";
        std::cout << "Peak usage: " << format_bytes(tracker.peak_usage_) << "\n";
        std::cout << "Total allocated: " << format_bytes(tracker.total_allocated_) << "\n";

        if (!tracker.allocations_.empty()) {
            std::cout << "\nBreakdown by category:\n";
            for (const auto& [desc, bytes] : tracker.allocations_) {
                std::cout << "  " << std::setw(30) << std::left << desc
                          << std::setw(15) << std::right << format_bytes(bytes) << "\n";
            }
        }

        std::cout << "\n";
    }

    /**
     * @brief Resets the tracker
     */
    static void reset() {
        auto& tracker = instance();
        tracker.current_usage_ = 0;
        tracker.peak_usage_ = 0;
        tracker.total_allocated_ = 0;
        tracker.allocations_.clear();
    }

private:
    static MemoryTracker& instance() {
        static MemoryTracker tracker;
        return tracker;
    }

    static std::string format_bytes(std::size_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB"};
        int unit_index = 0;
        double size = static_cast<double>(bytes);

        while (size >= 1024.0 && unit_index < 3) {
            size /= 1024.0;
            ++unit_index;
        }

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
        return oss.str();
    }

    MemoryTracker() = default;

    std::size_t current_usage_ = 0;
    std::size_t peak_usage_ = 0;
    std::size_t total_allocated_ = 0;
    std::unordered_map<std::string, std::size_t> allocations_;
};

/**
 * @brief Query explain utility
 */
class ExplainQuery {
public:
    /**
     * @brief Explains a simple table scan
     */
    static void table_scan(const std::string& table_name, std::size_t estimated_rows) {
        std::cout << "EXPLAIN: Sequential scan on table '" << table_name << "'\n";
        std::cout << "  Estimated rows: " << estimated_rows << "\n";
        std::cout << "  Cost: O(n) = " << estimated_rows << " row comparisons\n";
    }

    /**
     * @brief Explains an index scan
     */
    static void index_scan(const std::string& index_name, std::size_t estimated_rows) {
        std::cout << "EXPLAIN: Index scan using '" << index_name << "'\n";
        std::cout << "  Estimated rows: " << estimated_rows << "\n";
        std::cout << "  Cost: O(log n + k) = " << (std::log2(estimated_rows) + estimated_rows)
                  << " operations\n";
    }

    /**
     * @brief Explains a join
     */
    static void join(const std::string& join_type,
                    std::size_t left_rows,
                    std::size_t right_rows)
    {
        std::cout << "EXPLAIN: " << join_type << " join\n";
        std::cout << "  Left table rows: " << left_rows << "\n";
        std::cout << "  Right table rows: " << right_rows << "\n";
        std::cout << "  Estimated result rows: " << (left_rows * right_rows / 10) << "\n";
        std::cout << "  Cost: O(n + m) = " << (left_rows + right_rows) << " operations (hash join)\n";
    }
};

/**
 * @brief Macro for debug logging
 */
#define LEARNQL_DEBUG(msg, category) \
    learnql::debug::Logger::instance().debug(msg, category)

#define LEARNQL_INFO(msg, category) \
    learnql::debug::Logger::instance().info(msg, category)

#define LEARNQL_WARN(msg, category) \
    learnql::debug::Logger::instance().warn(msg, category)

#define LEARNQL_ERROR(msg, category) \
    learnql::debug::Logger::instance().error(msg, category)

} // namespace learnql::debug

#endif // LEARNQL_DEBUG_DEBUG_UTILS_HPP
