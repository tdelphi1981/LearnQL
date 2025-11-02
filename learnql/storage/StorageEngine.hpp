#ifndef LEARNQL_STORAGE_STORAGE_ENGINE_HPP
#define LEARNQL_STORAGE_STORAGE_ENGINE_HPP

#include "Page.hpp"
#include <string>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <stdexcept>
#include <algorithm>
#include <ctime>

namespace learnql::storage {

/**
 * @brief Storage engine managing pages and file I/O
 * @details Provides page-based storage with free list management
 *
 * Features:
 * - Page allocation and deallocation
 * - Free list management for reusing deleted pages
 * - Page caching for performance
 * - RAII file handling
 * - Transaction-like flush operations
 *
 * File Layout:
 * - Page 0: Metadata page (database info, free list head, etc.)
 * - Page 1+: Data pages
 */
class StorageEngine {
public:
    /**
     * @brief Creates or opens a database file
     * @param file_path Path to the database file
     * @param cache_size Number of pages to cache in memory
     * @throws std::runtime_error if file cannot be opened
     */
    explicit StorageEngine(const std::string& file_path, std::size_t cache_size = 64)
        : file_path_{file_path},
          next_page_id_{1},
          free_list_head_{0},
          sys_tables_root_{0},
          sys_fields_root_{0},
          sys_indexes_root_{0},
          cache_size_{cache_size},
          page_cache_{},
          dirty_pages_{} {

        // Create or open the file
        if (std::filesystem::exists(file_path_)) {
            load_metadata();
        } else {
            create_new_database();
        }
    }

    /**
     * @brief Destructor - flushes all dirty pages
     */
    ~StorageEngine() {
        try {
            flush_all();
        } catch (...) {
            // Suppress exceptions in destructor
        }
    }

    // Disable copy (file handle is unique)
    StorageEngine(const StorageEngine&) = delete;
    StorageEngine& operator=(const StorageEngine&) = delete;

    // Allow move
    StorageEngine(StorageEngine&&) noexcept = default;
    StorageEngine& operator=(StorageEngine&&) noexcept = default;

    /**
     * @brief Allocates a new page
     * @param type Type of page to allocate
     * @return Page ID of the allocated page
     * @details Uses free list if available, otherwise allocates new page
     */
    [[nodiscard]] uint64_t allocate_page(PageType type = PageType::DATA) {
        uint64_t page_id;

        // Try to reuse a free page
        if (free_list_head_ != 0) {
            page_id = free_list_head_;
            auto page = read_page(page_id);
            free_list_head_ = page.header().next_page_id;

            // Reset the page
            page = Page(page_id, type);
            write_page(page_id, page);
        } else {
            // Allocate a new page
            page_id = next_page_id_++;
            Page page(page_id, type);
            write_page(page_id, page);
        }

        // Update metadata
        save_metadata();

        return page_id;
    }

    /**
     * @brief Deallocates a page (adds to free list)
     * @param page_id ID of the page to deallocate
     */
    void deallocate_page(uint64_t page_id) {
        if (page_id == 0) {
            throw std::invalid_argument("Cannot deallocate metadata page");
        }

        // Read the page and mark it as free
        auto page = read_page(page_id);
        page.header().page_type = PageType::FREE;
        page.header().next_page_id = free_list_head_;
        page.clear();

        // Add to free list
        free_list_head_ = page_id;
        write_page(page_id, page);

        // Update metadata
        save_metadata();
    }

    /**
     * @brief Reads a page from storage
     * @param page_id ID of the page to read
     * @return The page
     * @throws std::runtime_error if page cannot be read
     */
    [[nodiscard]] Page read_page(uint64_t page_id) {
        // Check cache first
        auto it = page_cache_.find(page_id);
        if (it != page_cache_.end()) {
            return it->second;
        }

        // Read from file
        std::ifstream file(file_path_, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot open file for reading: " + file_path_);
        }

        // Seek to page
        file.seekg(static_cast<std::streamoff>(page_id * PAGE_SIZE));
        if (!file) {
            throw std::runtime_error("Cannot seek to page " + std::to_string(page_id));
        }

        // Read page
        Page page;
        file.read(static_cast<char*>(page.raw_data()), PAGE_SIZE);
        if (!file) {
            throw std::runtime_error("Cannot read page " + std::to_string(page_id));
        }

        // Validate page
        if (!page.header().is_valid()) {
            throw std::runtime_error("Invalid page header for page " + std::to_string(page_id));
        }

        // Add to cache
        if (page_cache_.size() >= cache_size_) {
            evict_page();
        }
        page_cache_[page_id] = page;

        return page;
    }

    /**
     * @brief Writes a page to storage
     * @param page_id ID of the page
     * @param page The page to write
     */
    void write_page(uint64_t page_id, const Page& page) {
        // Update cache
        page_cache_[page_id] = page;
        dirty_pages_.insert(page_id);

        // If cache is full, flush immediately
        if (dirty_pages_.size() > cache_size_ / 2) {
            flush_all();
        }
    }

    /**
     * @brief Flushes all dirty pages to disk
     */
    void flush_all() {
        if (dirty_pages_.empty()) {
            return;
        }

        std::fstream file(file_path_, std::ios::binary | std::ios::in | std::ios::out);
        if (!file) {
            throw std::runtime_error("Cannot open file for writing: " + file_path_);
        }

        for (uint64_t page_id : dirty_pages_) {
            auto it = page_cache_.find(page_id);
            if (it != page_cache_.end()) {
                // Update checksum before writing
                Page page = it->second;
                page.update_checksum();

                // Seek and write
                file.seekp(static_cast<std::streamoff>(page_id * PAGE_SIZE));
                file.write(static_cast<const char*>(page.raw_data()), PAGE_SIZE);

                if (!file) {
                    throw std::runtime_error("Cannot write page " + std::to_string(page_id));
                }

                // Update cache with checksummed page
                page_cache_[page_id] = page;
            }
        }

        file.flush();
        dirty_pages_.clear();
    }

    /**
     * @brief Flushes a specific page to disk
     * @param page_id ID of the page to flush
     */
    void flush_page(uint64_t page_id) {
        if (dirty_pages_.find(page_id) == dirty_pages_.end()) {
            return; // Not dirty
        }

        std::fstream file(file_path_, std::ios::binary | std::ios::in | std::ios::out);
        if (!file) {
            throw std::runtime_error("Cannot open file for writing: " + file_path_);
        }

        auto it = page_cache_.find(page_id);
        if (it != page_cache_.end()) {
            Page page = it->second;
            page.update_checksum();

            file.seekp(static_cast<std::streamoff>(page_id * PAGE_SIZE));
            file.write(static_cast<const char*>(page.raw_data()), PAGE_SIZE);

            if (!file) {
                throw std::runtime_error("Cannot write page " + std::to_string(page_id));
            }

            page_cache_[page_id] = page;
        }

        dirty_pages_.erase(page_id);
    }

    /**
     * @brief Gets the total number of pages
     * @return Total page count
     */
    [[nodiscard]] uint64_t get_page_count() const noexcept {
        return next_page_id_;
    }

    /**
     * @brief Gets the file path
     * @return Path to the database file
     */
    [[nodiscard]] const std::string& get_file_path() const noexcept {
        return file_path_;
    }

    /**
     * @brief Clears the page cache
     */
    void clear_cache() {
        flush_all();
        page_cache_.clear();
    }

    /**
     * @brief Get root page ID for _sys_tables catalog table
     * @return Root page ID, or 0 if not set
     */
    [[nodiscard]] uint64_t get_sys_tables_root() const {
        return sys_tables_root_;
    }

    /**
     * @brief Get root page ID for _sys_fields catalog table
     * @return Root page ID, or 0 if not set
     */
    [[nodiscard]] uint64_t get_sys_fields_root() const {
        return sys_fields_root_;
    }

    /**
     * @brief Set root page ID for _sys_tables catalog table
     * @param root_page_id Root page ID
     */
    void set_sys_tables_root(uint64_t root_page_id) {
        sys_tables_root_ = root_page_id;
        save_metadata();
    }

    /**
     * @brief Set root page ID for _sys_fields catalog table
     * @param root_page_id Root page ID
     */
    void set_sys_fields_root(uint64_t root_page_id) {
        sys_fields_root_ = root_page_id;
        save_metadata();
    }

    /**
     * @brief Get root page ID for _sys_indexes catalog table (NEW!)
     * @return Root page ID, or 0 if not set
     */
    [[nodiscard]] uint64_t get_sys_indexes_root() const {
        return sys_indexes_root_;
    }

    /**
     * @brief Set root page ID for _sys_indexes catalog table (NEW!)
     * @param root_page_id Root page ID
     */
    void set_sys_indexes_root(uint64_t root_page_id) {
        sys_indexes_root_ = root_page_id;
        save_metadata();
    }

private:

    /**
     * @brief Creates a new database file with metadata page (version 3 format)
     *
     * Page 0 Layout (New Format):
     * Offset 0-15:   "LearnQL Database" header (16 bytes)
     * Offset 16-23:  next_page_id (8 bytes)
     * Offset 24-31:  free_list_head (8 bytes)
     * Offset 32-39:  sys_tables_root page ID (8 bytes)
     * Offset 40-47:  sys_fields_root page ID (8 bytes)
     * Offset 48-51:  database version = 3 (4 bytes)  [UPDATED from v2]
     * Offset 52-59:  created_timestamp (8 bytes)
     * Offset 60-67:  sys_indexes_root page ID (8 bytes)  [NEW in v3]
     */
    void create_new_database() {
        // Create metadata page (page 0)
        Page metadata_page(0, PageType::METADATA);

        // Write database header
        std::array<char, 16> db_header = {'L', 'e', 'a', 'r', 'n', 'Q', 'L', ' ',
                                          'D', 'a', 't', 'a', 'b', 'a', 's', 'e'};
        metadata_page.write_data(0, db_header.data(), db_header.size());

        // Write metadata fields
        metadata_page.write_data(16, &next_page_id_, sizeof(next_page_id_));
        metadata_page.write_data(24, &free_list_head_, sizeof(free_list_head_));
        metadata_page.write_data(32, &sys_tables_root_, sizeof(sys_tables_root_));
        metadata_page.write_data(40, &sys_fields_root_, sizeof(sys_fields_root_));

        // Write database version (3 = includes secondary indexes support)
        uint32_t version = 3;
        metadata_page.write_data(48, &version, sizeof(version));

        // Write creation timestamp (Unix timestamp)
        uint64_t timestamp = static_cast<uint64_t>(std::time(nullptr));
        metadata_page.write_data(52, &timestamp, sizeof(timestamp));

        // Write sys_indexes_root (NEW in v3)
        metadata_page.write_data(60, &sys_indexes_root_, sizeof(sys_indexes_root_));

        // Create the file and write metadata page
        std::ofstream file(file_path_, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot create database file: " + file_path_);
        }

        metadata_page.update_checksum();
        file.write(static_cast<const char*>(metadata_page.raw_data()), PAGE_SIZE);
        file.close();
    }

    /**
     * @brief Loads metadata from page 0 (supports v2 and v3 formats)
     * @throws std::runtime_error if format is invalid or version is incompatible
     */
    void load_metadata() {
        Page metadata_page = read_page(0);

        // Validate database header
        std::array<char, 16> expected_header = {'L', 'e', 'a', 'r', 'n', 'Q', 'L', ' ',
                                                 'D', 'a', 't', 'a', 'b', 'a', 's', 'e'};
        std::array<char, 16> actual_header;
        metadata_page.read_data(0, actual_header.data(), actual_header.size());

        if (actual_header != expected_header) {
            throw std::runtime_error("Invalid database file format");
        }

        // Read metadata fields
        metadata_page.read_data(16, &next_page_id_, sizeof(next_page_id_));
        metadata_page.read_data(24, &free_list_head_, sizeof(free_list_head_));
        metadata_page.read_data(32, &sys_tables_root_, sizeof(sys_tables_root_));
        metadata_page.read_data(40, &sys_fields_root_, sizeof(sys_fields_root_));

        // Read and validate version
        uint32_t version = 0;
        metadata_page.read_data(48, &version, sizeof(version));

        if (version == 2) {
            // Version 2: No secondary indexes support
            sys_indexes_root_ = 0;  // Will be created on first index creation
        } else if (version == 3) {
            // Version 3: Secondary indexes supported
            metadata_page.read_data(60, &sys_indexes_root_, sizeof(sys_indexes_root_));
        } else {
            throw std::runtime_error(
                "Incompatible database version: " + std::to_string(version) +
                " (expected version 2 or 3). Please recreate the database."
            );
        }

        // Note: created_timestamp at offset 52 is not read (not needed for runtime)
    }

    /**
     * @brief Saves metadata to page 0 (supports v2 and v3 formats)
     */
    void save_metadata() {
        Page metadata_page = read_page(0);

        // Write metadata fields
        metadata_page.write_data(16, &next_page_id_, sizeof(next_page_id_));
        metadata_page.write_data(24, &free_list_head_, sizeof(free_list_head_));
        metadata_page.write_data(32, &sys_tables_root_, sizeof(sys_tables_root_));
        metadata_page.write_data(40, &sys_fields_root_, sizeof(sys_fields_root_));

        // Write sys_indexes_root (only in v3, but safe to write regardless)
        metadata_page.write_data(60, &sys_indexes_root_, sizeof(sys_indexes_root_));

        // Note: version and timestamp are written once during create_new_database()
        // and don't need to be updated on every save

        write_page(0, metadata_page);
    }

    /**
     * @brief Evicts a page from the cache (LRU-like)
     */
    void evict_page() {
        if (page_cache_.empty()) {
            return;
        }

        // Simple eviction: remove first non-dirty page
        for (auto it = page_cache_.begin(); it != page_cache_.end(); ++it) {
            if (dirty_pages_.find(it->first) == dirty_pages_.end()) {
                page_cache_.erase(it);
                return;
            }
        }

        // If all pages are dirty, flush and evict the first one
        uint64_t page_id = page_cache_.begin()->first;
        flush_page(page_id);
        page_cache_.erase(page_id);
    }

private:
    std::string file_path_;                         ///< Path to database file
    uint64_t next_page_id_;                         ///< Next page ID to allocate
    uint64_t free_list_head_;                       ///< Head of free list
    uint64_t sys_tables_root_;                      ///< Root page ID for _sys_tables
    uint64_t sys_fields_root_;                      ///< Root page ID for _sys_fields
    uint64_t sys_indexes_root_;                     ///< Root page ID for _sys_indexes (NEW!)
    std::size_t cache_size_;                        ///< Maximum cache size
    std::unordered_map<uint64_t, Page> page_cache_; ///< Page cache
    std::unordered_set<uint64_t> dirty_pages_;      ///< Set of dirty page IDs
};

} // namespace learnql::storage

#endif // LEARNQL_STORAGE_STORAGE_ENGINE_HPP
