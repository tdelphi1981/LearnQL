#ifndef LEARNQL_CORE_RECORD_ID_HPP
#define LEARNQL_CORE_RECORD_ID_HPP

#include <cstdint>
#include <compare>
#include <functional>

namespace learnql::core {

/**
 * @brief Type-safe record identifier
 * @details Represents the physical location of a record in storage
 *
 * A RecordId consists of:
 * - Page ID: Which page the record is on
 * - Slot: Which slot within the page (for future slotted page architecture)
 *
 * Features:
 * - Type-safe (cannot mix record IDs from different tables)
 * - Three-way comparison support
 * - Hashable for use in unordered containers
 *
 * Example:
 * @code
 * RecordId id{.page_id = 5, .slot = 3};
 * if (id.is_valid()) {
 *     // Use the ID
 * }
 * @endcode
 */
struct RecordId {
    uint64_t page_id;  ///< Page where record is stored
    uint32_t slot;     ///< Slot within the page

    /**
     * @brief Default constructor - creates invalid ID
     */
    constexpr RecordId() noexcept : page_id(0), slot(0) {}

    /**
     * @brief Constructs a record ID
     * @param pid Page ID
     * @param s Slot number
     */
    constexpr RecordId(uint64_t pid, uint32_t s) noexcept
        : page_id(pid), slot(s) {}

    /**
     * @brief Checks if the ID is valid
     * @return true if both page_id and slot are non-zero
     */
    [[nodiscard]] constexpr bool is_valid() const noexcept {
        return page_id != 0;  // Page 0 is metadata, so never contains user data
    }

    /**
     * @brief Creates an invalid record ID
     */
    [[nodiscard]] static constexpr RecordId invalid() noexcept {
        return RecordId{0, 0};
    }

    /**
     * @brief Three-way comparison
     */
    auto operator<=>(const RecordId&) const noexcept = default;

    /**
     * @brief Equality comparison
     */
    bool operator==(const RecordId&) const noexcept = default;
};

} // namespace learnql::core

/**
 * @brief Hash specialization for RecordId
 * @details Enables use in std::unordered_map and std::unordered_set
 */
template<>
struct std::hash<learnql::core::RecordId> {
    std::size_t operator()(const learnql::core::RecordId& id) const noexcept {
        // Combine page_id and slot into a single hash
        std::size_t h1 = std::hash<uint64_t>{}(id.page_id);
        std::size_t h2 = std::hash<uint32_t>{}(id.slot);
        return h1 ^ (h2 << 1);  // Simple hash combination
    }
};

#endif // LEARNQL_CORE_RECORD_ID_HPP
