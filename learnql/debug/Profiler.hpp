#ifndef LEARNQL_DEBUG_PROFILER_HPP
#define LEARNQL_DEBUG_PROFILER_HPP

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace learnql::debug {

/**
 * @brief Performance metrics for a query or operation
 */
struct PerformanceMetrics {
    std::string operation_name;
    std::chrono::microseconds duration;
    std::size_t rows_processed;
    std::size_t memory_used;

    [[nodiscard]] double duration_ms() const {
        return duration.count() / 1000.0;
    }

    [[nodiscard]] double rows_per_second() const {
        if (duration.count() == 0) return 0.0;
        return (rows_processed * 1'000'000.0) / duration.count();
    }
};

/**
 * @brief RAII timer for profiling code blocks
 *
 * Example:
 * @code
 * {
 *     Timer timer("Query execution");
 *     // ... code to profile ...
 * }  // Automatically prints duration when timer goes out of scope
 * @endcode
 */
class Timer {
public:
    /**
     * @brief Constructs and starts the timer
     * @param name Name of the operation being timed
     */
    explicit Timer(std::string name)
        : name_(std::move(name)),
          start_(std::chrono::high_resolution_clock::now()) {}

    /**
     * @brief Destructor - prints elapsed time
     */
    ~Timer() {
        if (!stopped_) {
            stop();
            print();
        }
    }

    /**
     * @brief Stops the timer and returns elapsed microseconds
     */
    std::chrono::microseconds stop() {
        if (!stopped_) {
            end_ = std::chrono::high_resolution_clock::now();
            stopped_ = true;
        }
        return std::chrono::duration_cast<std::chrono::microseconds>(end_ - start_);
    }

    /**
     * @brief Prints the elapsed time
     */
    void print() const {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_ - start_);
        std::cout << "[TIMER] " << name_ << ": "
                  << std::fixed << std::setprecision(3)
                  << (duration.count() / 1000.0) << " ms\n";
    }

private:
    std::string name_;
    std::chrono::high_resolution_clock::time_point start_;
    std::chrono::high_resolution_clock::time_point end_;
    bool stopped_ = false;
};

/**
 * @brief Profiler for collecting and analyzing performance metrics
 *
 * Singleton profiler that collects metrics from multiple operations
 * and can generate summary reports.
 */
class Profiler {
public:
    /**
     * @brief Gets the singleton instance
     */
    static Profiler& instance() {
        static Profiler profiler;
        return profiler;
    }

    /**
     * @brief Records a performance metric
     */
    void record(const PerformanceMetrics& metrics) {
        metrics_.push_back(metrics);
        operation_metrics_[metrics.operation_name].push_back(metrics);
    }

    /**
     * @brief Prints a summary of all metrics
     */
    void print_summary() const {
        if (metrics_.empty()) {
            std::cout << "No performance metrics recorded.\n";
            return;
        }

        std::cout << "\n=== Performance Summary ===\n\n";

        // Overall statistics
        auto total_duration = std::chrono::microseconds::zero();
        std::size_t total_rows = 0;

        for (const auto& m : metrics_) {
            total_duration += m.duration;
            total_rows += m.rows_processed;
        }

        std::cout << "Total operations: " << metrics_.size() << "\n";
        std::cout << "Total duration: " << std::fixed << std::setprecision(3)
                  << (total_duration.count() / 1000.0) << " ms\n";
        std::cout << "Total rows processed: " << total_rows << "\n";
        std::cout << "Overall throughput: " << std::fixed << std::setprecision(2)
                  << (total_rows * 1'000'000.0 / total_duration.count()) << " rows/sec\n\n";

        // Per-operation statistics
        std::cout << "Per-operation breakdown:\n";
        std::cout << std::setw(30) << std::left << "Operation"
                  << std::setw(12) << std::right << "Count"
                  << std::setw(15) << "Total (ms)"
                  << std::setw(15) << "Avg (ms)"
                  << std::setw(15) << "Rows"
                  << std::setw(15) << "Rows/sec\n";
        std::cout << std::string(102, '-') << "\n";

        for (const auto& [op_name, op_metrics] : operation_metrics_) {
            auto op_total_duration = std::chrono::microseconds::zero();
            std::size_t op_total_rows = 0;

            for (const auto& m : op_metrics) {
                op_total_duration += m.duration;
                op_total_rows += m.rows_processed;
            }

            double avg_duration = op_total_duration.count() / (1000.0 * op_metrics.size());
            double rows_per_sec = (op_total_rows * 1'000'000.0) / op_total_duration.count();

            std::cout << std::setw(30) << std::left << op_name
                      << std::setw(12) << std::right << op_metrics.size()
                      << std::setw(15) << std::fixed << std::setprecision(3)
                      << (op_total_duration.count() / 1000.0)
                      << std::setw(15) << avg_duration
                      << std::setw(15) << op_total_rows
                      << std::setw(15) << std::fixed << std::setprecision(2) << rows_per_sec
                      << "\n";
        }

        std::cout << "\n";
    }

    /**
     * @brief Clears all recorded metrics
     */
    void clear() {
        metrics_.clear();
        operation_metrics_.clear();
    }

    /**
     * @brief Gets all metrics
     */
    [[nodiscard]] const std::vector<PerformanceMetrics>& metrics() const noexcept {
        return metrics_;
    }

private:
    Profiler() = default;
    ~Profiler() = default;

    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;

    std::vector<PerformanceMetrics> metrics_;
    std::unordered_map<std::string, std::vector<PerformanceMetrics>> operation_metrics_;
};

/**
 * @brief RAII class for profiling a scope and automatically recording metrics
 *
 * Example:
 * @code
 * {
 *     ScopedProfiler prof("Table scan");
 *     // ... code to profile ...
 *     prof.set_rows_processed(1000);
 * }  // Automatically records metrics when it goes out of scope
 * @endcode
 */
class ScopedProfiler {
public:
    /**
     * @brief Constructs and starts profiling
     * @param operation_name Name of the operation
     */
    explicit ScopedProfiler(std::string operation_name)
        : operation_name_(std::move(operation_name)),
          start_(std::chrono::high_resolution_clock::now()),
          rows_processed_(0),
          memory_used_(0) {}

    /**
     * @brief Destructor - records metrics
     */
    ~ScopedProfiler() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);

        PerformanceMetrics metrics{
            operation_name_,
            duration,
            rows_processed_,
            memory_used_
        };

        Profiler::instance().record(metrics);
    }

    /**
     * @brief Sets the number of rows processed
     */
    void set_rows_processed(std::size_t rows) {
        rows_processed_ = rows;
    }

    /**
     * @brief Sets the memory used
     */
    void set_memory_used(std::size_t bytes) {
        memory_used_ = bytes;
    }

private:
    std::string operation_name_;
    std::chrono::high_resolution_clock::time_point start_;
    std::size_t rows_processed_;
    std::size_t memory_used_;
};

} // namespace learnql::debug

#endif // LEARNQL_DEBUG_PROFILER_HPP
