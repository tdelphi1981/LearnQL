#ifndef LEARNQL_STORAGE_PAGE_HPP
#define LEARNQL_STORAGE_PAGE_HPP

#include <cstdint>
#include <cstring>
#include <array>
#include <span>
#include <stdexcept>
#include <compare>

namespace learnql::storage {

/**
 * @brief Magic number for LearnQL database files
 * @details "LQL" + version marker
 */
constexpr std::array<char, 4> MAGIC_NUMBER = {'L', 'Q', 'L', '1'};

/**
 * @brief Standard page size (4KB)
 * @details Common page size for efficient disk I/O
 */
constexpr std::size_t PAGE_SIZE = 4096;

/**
 * @brief Types of pages in the storage system
 */
enum class PageType : uint8_t {
    FREE = 0,        ///< Unused page
    DATA = 1,        ///< Contains record data
    INDEX = 2,       ///< Contains index data (future use)
    METADATA = 3,    ///< Contains database metadata
    OVERFLOW_DATA = 4 ///< Contains overflow data for large records (renamed to avoid macro conflict)
};

/**
 * @brief Page header structure (64 bytes)
 * @details Contains metadata about the page
 *
 * Layout:
 * - Magic number: 4 bytes (validation)
 * - Page ID: 8 bytes (unique identifier)
 * - Page type: 1 byte
 * - Version: 1 byte (for future compatibility)
 * - Record count: 2 bytes (number of records in page)
 * - Free space offset: 2 bytes (where free space begins)
 * - Next page: 8 bytes (for linked lists)
 * - Checksum: 4 bytes (for integrity checking)
 * - Reserved: 34 bytes (for future use)
 */
#pragma pack(push, 1)
struct PageHeader {
    std::array<char, 4> magic;       ///< Magic number for validation
    uint64_t page_id;                ///< Unique page identifier
    PageType page_type;              ///< Type of page
    uint8_t version;                 ///< Format version
    uint16_t record_count;           ///< Number of records in this page
    uint16_t free_space_offset;      ///< Offset to free space
    uint64_t next_page_id;           ///< Next page in chain (0 = none)
    uint32_t checksum;               ///< CRC32 checksum
    std::array<uint8_t, 34> reserved; ///< Reserved for future use

    /**
     * @brief Default constructor - initializes a free page
     */
    PageHeader() noexcept
        : magic(MAGIC_NUMBER),
          page_id(0),
          page_type(PageType::FREE),
          version(1),
          record_count(0),
          free_space_offset(sizeof(PageHeader)),
          next_page_id(0),
          checksum(0),
          reserved{} {}

    /**
     * @brief Validates the magic number
     * @return true if magic number is valid
     */
    [[nodiscard]] bool is_valid() const noexcept {
        return magic == MAGIC_NUMBER;
    }

    /**
     * @brief Three-way comparison for page headers
     */
    auto operator<=>(const PageHeader& other) const noexcept = default;
};
#pragma pack(pop)

// Ensure PageHeader is exactly 64 bytes
static_assert(sizeof(PageHeader) == 64, "PageHeader must be exactly 64 bytes");

/**
 * @brief A single page in the storage system (4KB)
 * @details Fixed-size page containing header and data
 *
 * Layout:
 * - Header: 64 bytes (PageHeader)
 * - Data: 4032 bytes (PAGE_SIZE - sizeof(PageHeader))
 *
 * Features:
 * - RAII-based memory management
 * - Safe data access via std::span
 * - Compile-time size validation
 * - C++20 three-way comparison
 */
class Page {
public:
    /**
     * @brief Size of data section
     */
    static constexpr std::size_t DATA_SIZE = PAGE_SIZE - sizeof(PageHeader);

    /**
     * @brief Default constructor - creates a free page
     */
    Page() noexcept : header_{}, data_{} {}

    /**
     * @brief Constructs a page with specific type and ID
     * @param page_id Unique page identifier
     * @param type Type of page
     */
    explicit Page(uint64_t page_id, PageType type = PageType::DATA) noexcept
        : header_{}, data_{} {
        header_.page_id = page_id;
        header_.page_type = type;
    }

    // Default copy operations (needed for caching)
    Page(const Page&) = default;
    Page& operator=(const Page&) = default;

    // Default move operations
    Page(Page&&) noexcept = default;
    Page& operator=(Page&&) noexcept = default;

    /**
     * @brief Gets the page header
     * @return Reference to the header
     */
    [[nodiscard]] PageHeader& header() noexcept {
        return header_;
    }

    /**
     * @brief Gets the page header (const)
     * @return Const reference to the header
     */
    [[nodiscard]] const PageHeader& header() const noexcept {
        return header_;
    }

    /**
     * @brief Gets a view of the data section
     * @return std::span providing safe access to data
     */
    [[nodiscard]] std::span<uint8_t> data() noexcept {
        return std::span<uint8_t>(data_.data(), data_.size());
    }

    /**
     * @brief Gets a view of the data section (const)
     * @return std::span providing safe read-only access to data
     */
    [[nodiscard]] std::span<const uint8_t> data() const noexcept {
        return std::span<const uint8_t>(data_.data(), data_.size());
    }

    /**
     * @brief Writes data to the page
     * @param offset Offset in data section to write to
     * @param src Source data
     * @param size Number of bytes to write
     * @throws std::out_of_range if offset + size exceeds page size
     */
    void write_data(std::size_t offset, const void* src, std::size_t size) {
        if (offset + size > DATA_SIZE) {
            throw std::out_of_range("Write exceeds page boundary");
        }
        std::memcpy(data_.data() + offset, src, size);
    }

    /**
     * @brief Reads data from the page
     * @param offset Offset in data section to read from
     * @param dest Destination buffer
     * @param size Number of bytes to read
     * @throws std::out_of_range if offset + size exceeds page size
     */
    void read_data(std::size_t offset, void* dest, std::size_t size) const {
        if (offset + size > DATA_SIZE) {
            throw std::out_of_range("Read exceeds page boundary");
        }
        std::memcpy(dest, data_.data() + offset, size);
    }

    /**
     * @brief Clears the page (resets to free state)
     */
    void clear() noexcept {
        header_ = PageHeader{};
        data_.fill(0);
    }

    /**
     * @brief Calculates available free space in bytes
     * @return Number of bytes available
     */
    [[nodiscard]] std::size_t available_space() const noexcept {
        return DATA_SIZE - header_.free_space_offset + sizeof(PageHeader);
    }

    /**
     * @brief Checks if the page can fit data of given size
     * @param size Size in bytes
     * @return true if data fits
     */
    [[nodiscard]] bool can_fit(std::size_t size) const noexcept {
        return available_space() >= size;
    }

    /**
     * @brief Gets raw pointer to page data (for I/O operations)
     * @return Pointer to beginning of page (header)
     */
    [[nodiscard]] const void* raw_data() const noexcept {
        return &header_;
    }

    /**
     * @brief Gets raw pointer to page data (for I/O operations)
     * @return Pointer to beginning of page (header)
     */
    [[nodiscard]] void* raw_data() noexcept {
        return &header_;
    }

    /**
     * @brief Computes CRC32 checksum of page data
     * @return Checksum value
     */
    [[nodiscard]] uint32_t compute_checksum() const noexcept {
        uint32_t checksum = 0;
        // Simple XOR-based checksum for educational purposes
        // In production, use proper CRC32
        for (const auto& byte : data_) {
            checksum ^= byte;
        }
        return checksum;
    }

    /**
     * @brief Updates the checksum in the header
     */
    void update_checksum() noexcept {
        header_.checksum = compute_checksum();
    }

    /**
     * @brief Validates the checksum
     * @return true if checksum is valid
     */
    [[nodiscard]] bool validate_checksum() const noexcept {
        return header_.checksum == compute_checksum();
    }

private:
    PageHeader header_;                ///< Page metadata
    std::array<uint8_t, DATA_SIZE> data_; ///< Page data section
};

// Ensure Page is exactly PAGE_SIZE bytes
static_assert(sizeof(Page) == PAGE_SIZE, "Page must be exactly PAGE_SIZE bytes");

} // namespace learnql::storage

#endif // LEARNQL_STORAGE_PAGE_HPP
