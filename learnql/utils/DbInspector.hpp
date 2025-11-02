#ifndef LEARNQL_UTILS_DB_INSPECTOR_HPP
#define LEARNQL_UTILS_DB_INSPECTOR_HPP

#include "../storage/StorageEngine.hpp"
#include "../storage/Page.hpp"
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <sstream>

namespace learnql::utils {

/**
 * @brief Database structure inspector and visualization utility
 *
 * Provides functions to analyze and display the internal structure
 * of a LearnQL database file, including page allocation, index structure,
 * and storage statistics.
 */
class DbInspector {
public:
    /**
     * @brief Page information structure
     */
    struct PageInfo {
        uint64_t page_id;
        storage::PageType type;
        uint16_t record_count;
        uint16_t free_space_offset;
        std::size_t used_space;
        std::size_t free_space;
        std::string owner_table;  // For INDEX/DATA pages
    };

    /**
     * @brief Storage statistics
     */
    struct StorageStats {
        std::size_t total_pages = 0;
        std::size_t metadata_pages = 0;
        std::size_t data_pages = 0;
        std::size_t index_pages = 0;
        std::size_t free_pages = 0;
        std::size_t total_used_space = 0;
        std::size_t total_free_space = 0;
        std::map<std::string, std::size_t> table_data_pages;
        std::map<std::string, std::size_t> table_index_pages;
    };

    /**
     * @brief Prints the complete database structure
     * @param storage Storage engine to inspect
     * @param show_details If true, shows detailed information for each page
     */
    static void print_database_structure(storage::StorageEngine& storage, bool show_details = false) {
        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║              DATABASE STRUCTURE INSPECTION                         ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";

        // Collect page information
        auto pages = collect_page_info(storage);
        auto stats = calculate_statistics(pages);

        // Print summary
        print_summary(stats);

        // Print page type breakdown
        print_page_breakdown(stats);

        // Print table information
        print_table_info(stats);

        if (show_details) {
            // Print detailed page listing
            print_page_details(pages);
        }

        // Print storage efficiency
        print_storage_efficiency(stats);
    }

    /**
     * @brief Prints a compact summary of database structure
     * @param storage Storage engine to inspect
     */
    static void print_compact_summary(storage::StorageEngine& storage) {
        auto pages = collect_page_info(storage);
        auto stats = calculate_statistics(pages);

        std::cout << "Database Summary: "
                  << stats.total_pages << " pages | "
                  << stats.data_pages << " DATA | "
                  << stats.index_pages << " INDEX | "
                  << stats.free_pages << " FREE | "
                  << format_bytes(stats.total_used_space) << " used\n";
    }

    /**
     * @brief Prints page allocation map (visual representation)
     * @param storage Storage engine to inspect
     * @param pages_per_row Number of pages to display per row
     */
    static void print_page_map(storage::StorageEngine& storage, std::size_t pages_per_row = 32) {
        auto pages = collect_page_info(storage);

        std::cout << "\nPage Allocation Map:\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "Legend: [M]=Metadata [D]=Data [I]=Index [O]=Overflow [F]=Free\n\n";

        for (std::size_t i = 0; i < pages.size(); ++i) {
            if (i % pages_per_row == 0) {
                if (i > 0) std::cout << "\n";
                std::cout << std::setw(6) << i << ": ";
            }

            char symbol = '?';
            switch (pages[i].type) {
                case storage::PageType::METADATA:      symbol = 'M'; break;
                case storage::PageType::DATA:          symbol = 'D'; break;
                case storage::PageType::INDEX:         symbol = 'I'; break;
                case storage::PageType::FREE:          symbol = 'F'; break;
                case storage::PageType::OVERFLOW_DATA: symbol = 'O'; break;
            }
            std::cout << symbol;
        }
        std::cout << "\n\n";
    }

private:
    /**
     * @brief Collects information about all pages in the database
     */
    static std::vector<PageInfo> collect_page_info(storage::StorageEngine& storage) {
        std::vector<PageInfo> pages;

        // Try to read pages until we hit an error or reach a reasonable limit
        for (uint64_t page_id = 0; page_id < 1000; ++page_id) {
            try {
                auto page = storage.read_page(page_id);
                PageInfo info;
                info.page_id = page_id;
                info.type = page.header().page_type;
                info.record_count = page.header().record_count;
                info.free_space_offset = page.header().free_space_offset;
                info.used_space = page.header().free_space_offset;
                info.free_space = storage::Page::DATA_SIZE - page.header().free_space_offset;
                pages.push_back(info);
            } catch (...) {
                // End of valid pages
                break;
            }
        }

        return pages;
    }

    /**
     * @brief Calculates storage statistics from page information
     */
    static StorageStats calculate_statistics(const std::vector<PageInfo>& pages) {
        StorageStats stats;
        stats.total_pages = pages.size();

        for (const auto& page : pages) {
            switch (page.type) {
                case storage::PageType::METADATA:
                    stats.metadata_pages++;
                    break;
                case storage::PageType::DATA:
                    stats.data_pages++;
                    break;
                case storage::PageType::INDEX:
                    stats.index_pages++;
                    break;
                case storage::PageType::FREE:
                    stats.free_pages++;
                    break;
                case storage::PageType::OVERFLOW_DATA:
                    stats.data_pages++;  // Count as data pages
                    break;
            }

            stats.total_used_space += page.used_space;
            stats.total_free_space += page.free_space;
        }

        return stats;
    }

    /**
     * @brief Prints summary statistics
     */
    static void print_summary(const StorageStats& stats) {
        std::cout << "Summary\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "Total Pages:        " << std::setw(8) << stats.total_pages << "\n";
        std::cout << "Database Size:      " << std::setw(8) << format_bytes(stats.total_pages * storage::PAGE_SIZE) << "\n";
        std::cout << "Used Space:         " << std::setw(8) << format_bytes(stats.total_used_space) << "\n";
        std::cout << "Free Space:         " << std::setw(8) << format_bytes(stats.total_free_space) << "\n";
        std::cout << "\n";
    }

    /**
     * @brief Prints page type breakdown
     */
    static void print_page_breakdown(const StorageStats& stats) {
        std::cout << "Page Type Breakdown\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << std::left;
        std::cout << std::setw(20) << "Type"
                  << std::setw(12) << "Count"
                  << std::setw(12) << "Percentage"
                  << std::setw(15) << "Size"
                  << "\n";
        std::cout << std::string(80, '-') << "\n";

        auto print_row = [&](const std::string& type, std::size_t count) {
            double percentage = (stats.total_pages > 0)
                ? (static_cast<double>(count) / stats.total_pages * 100.0)
                : 0.0;
            std::size_t size = count * storage::PAGE_SIZE;

            std::cout << std::setw(20) << type
                      << std::setw(12) << count
                      << std::setw(10) << std::fixed << std::setprecision(1) << percentage << "%"
                      << std::setw(15) << format_bytes(size)
                      << "\n";
        };

        print_row("METADATA", stats.metadata_pages);
        print_row("DATA", stats.data_pages);
        print_row("INDEX", stats.index_pages);
        print_row("FREE", stats.free_pages);
        std::cout << std::string(80, '-') << "\n";
        std::cout << std::setw(20) << "TOTAL"
                  << std::setw(12) << stats.total_pages
                  << std::setw(10) << "100.0%"
                  << std::setw(15) << format_bytes(stats.total_pages * storage::PAGE_SIZE)
                  << "\n";
        std::cout << "\n";
    }

    /**
     * @brief Prints table-specific information
     */
    static void print_table_info(const StorageStats& stats) {
        if (stats.table_data_pages.empty() && stats.table_index_pages.empty()) {
            return;
        }

        std::cout << "Table Storage Information\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << std::left;
        std::cout << std::setw(20) << "Table"
                  << std::setw(15) << "Data Pages"
                  << std::setw(15) << "Index Pages"
                  << std::setw(15) << "Total Pages"
                  << "\n";
        std::cout << std::string(80, '-') << "\n";

        // Combine table names from both maps
        std::set<std::string> all_tables;
        for (const auto& [table, _] : stats.table_data_pages) {
            all_tables.insert(table);
        }
        for (const auto& [table, _] : stats.table_index_pages) {
            all_tables.insert(table);
        }

        for (const auto& table : all_tables) {
            std::size_t data_pages = 0;
            std::size_t index_pages = 0;

            auto dit = stats.table_data_pages.find(table);
            if (dit != stats.table_data_pages.end()) {
                data_pages = dit->second;
            }

            auto iit = stats.table_index_pages.find(table);
            if (iit != stats.table_index_pages.end()) {
                index_pages = iit->second;
            }

            std::cout << std::setw(20) << table
                      << std::setw(15) << data_pages
                      << std::setw(15) << index_pages
                      << std::setw(15) << (data_pages + index_pages)
                      << "\n";
        }
        std::cout << "\n";
    }

    /**
     * @brief Prints detailed page listing
     */
    static void print_page_details(const std::vector<PageInfo>& pages) {
        std::cout << "Detailed Page Listing\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << std::left;
        std::cout << std::setw(10) << "Page ID"
                  << std::setw(12) << "Type"
                  << std::setw(12) << "Records"
                  << std::setw(15) << "Used"
                  << std::setw(15) << "Free"
                  << "\n";
        std::cout << std::string(80, '-') << "\n";

        for (const auto& page : pages) {
            std::string type_str;
            switch (page.type) {
                case storage::PageType::METADATA:      type_str = "METADATA"; break;
                case storage::PageType::DATA:          type_str = "DATA"; break;
                case storage::PageType::INDEX:         type_str = "INDEX"; break;
                case storage::PageType::FREE:          type_str = "FREE"; break;
                case storage::PageType::OVERFLOW_DATA: type_str = "OVERFLOW"; break;
            }

            std::cout << std::setw(10) << page.page_id
                      << std::setw(12) << type_str
                      << std::setw(12) << page.record_count
                      << std::setw(15) << format_bytes(page.used_space)
                      << std::setw(15) << format_bytes(page.free_space)
                      << "\n";
        }
        std::cout << "\n";
    }

    /**
     * @brief Prints storage efficiency metrics
     */
    static void print_storage_efficiency(const StorageStats& stats) {
        std::cout << "Storage Efficiency\n";
        std::cout << std::string(80, '-') << "\n";

        std::size_t total_space = stats.total_pages * storage::Page::DATA_SIZE;
        double utilization = (total_space > 0)
            ? (static_cast<double>(stats.total_used_space) / total_space * 100.0)
            : 0.0;

        std::cout << "Total Capacity:     " << format_bytes(total_space) << "\n";
        std::cout << "Used Space:         " << format_bytes(stats.total_used_space) << "\n";
        std::cout << "Free Space:         " << format_bytes(stats.total_free_space) << "\n";
        std::cout << "Utilization:        " << std::fixed << std::setprecision(1) << utilization << "%\n";
        std::cout << "\n";

        // Space distribution
        std::size_t data_space = stats.data_pages * storage::PAGE_SIZE;
        std::size_t index_space = stats.index_pages * storage::PAGE_SIZE;
        std::size_t overhead_space = (stats.metadata_pages + stats.free_pages) * storage::PAGE_SIZE;

        std::cout << "Space Distribution:\n";
        std::cout << "  Data Pages:       " << format_bytes(data_space)
                  << " (" << std::fixed << std::setprecision(1)
                  << (total_space > 0 ? static_cast<double>(data_space) / total_space * 100.0 : 0.0) << "%)\n";
        std::cout << "  Index Pages:      " << format_bytes(index_space)
                  << " (" << std::fixed << std::setprecision(1)
                  << (total_space > 0 ? static_cast<double>(index_space) / total_space * 100.0 : 0.0) << "%)\n";
        std::cout << "  Overhead/Free:    " << format_bytes(overhead_space)
                  << " (" << std::fixed << std::setprecision(1)
                  << (total_space > 0 ? static_cast<double>(overhead_space) / total_space * 100.0 : 0.0) << "%)\n";
        std::cout << "\n";
    }

    /**
     * @brief Formats bytes into human-readable format
     */
    static std::string format_bytes(std::size_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB"};
        int unit_index = 0;
        double size = static_cast<double>(bytes);

        while (size >= 1024.0 && unit_index < 3) {
            size /= 1024.0;
            unit_index++;
        }

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
        return oss.str();
    }
};

} // namespace learnql::utils

#endif // LEARNQL_UTILS_DB_INSPECTOR_HPP
