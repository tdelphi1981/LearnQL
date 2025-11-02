#ifndef LEARNQL_INDEX_PERSISTENT_MULTI_VALUE_SECONDARY_INDEX_HPP
#define LEARNQL_INDEX_PERSISTENT_MULTI_VALUE_SECONDARY_INDEX_HPP

#include "PersistentBTreeIndex.hpp"
#include "../core/RecordId.hpp"
#include "../storage/StorageEngine.hpp"
#include <functional>
#include <vector>
#include <string>
#include <optional>
#include <memory>
#include <limits>
#include <compare>

namespace learnql::index {

/**
 * @brief Composite key for multi-value secondary indexes
 * @tparam FieldType Type of the indexed field
 *
 * Combines field value with page ID to create unique composite keys.
 * This allows multiple records with the same field value.
 *
 * Example: If two users have email "test@example.com":
 * - User 1 (page 10): composite key = ("test@example.com", 10)
 * - User 2 (page 20): composite key = ("test@example.com", 20)
 *
 * Both stored as separate B+Tree entries, enabling range queries.
 */
template<typename FieldType>
struct CompositeKey {
    FieldType field_value;
    uint64_t page_id;

    CompositeKey() : field_value{}, page_id(0) {}

    CompositeKey(FieldType value, uint64_t pid)
        : field_value(std::move(value)), page_id(pid) {}

    // Comparison operators for B+Tree ordering
    // Note: Using explicit comparisons for better compatibility
    bool operator==(const CompositeKey& other) const {
        return field_value == other.field_value && page_id == other.page_id;
    }

    bool operator!=(const CompositeKey& other) const {
        return !(*this == other);
    }

    bool operator<(const CompositeKey& other) const {
        if (field_value < other.field_value) return true;
        if (other.field_value < field_value) return false;
        return page_id < other.page_id;
    }

    bool operator<=(const CompositeKey& other) const {
        return !(other < *this);
    }

    bool operator>(const CompositeKey& other) const {
        return other < *this;
    }

    bool operator>=(const CompositeKey& other) const {
        return !(*this < other);
    }

    // C++20 spaceship operator for modern code
    auto operator<=>(const CompositeKey& other) const {
        if (field_value < other.field_value) return std::strong_ordering::less;
        if (other.field_value < field_value) return std::strong_ordering::greater;
        if (page_id < other.page_id) return std::strong_ordering::less;
        if (page_id > other.page_id) return std::strong_ordering::greater;
        return std::strong_ordering::equal;
    }

    // Serialization support
    template<typename Writer>
    void serialize(Writer& writer) const {
        if constexpr (std::is_arithmetic_v<FieldType>) {
            writer.write(field_value);
        } else if constexpr (requires { std::string(field_value); }) {
            writer.write(std::string(field_value));
        } else {
            writer.write(field_value);
        }
        writer.write(page_id);
    }

    template<typename Reader>
    void deserialize(Reader& reader) {
        if constexpr (std::is_arithmetic_v<FieldType>) {
            field_value = reader.template read<FieldType>();
        } else if constexpr (requires { std::string(FieldType{}); }) {
            field_value = FieldType(reader.read_string());
        } else {
            field_value.deserialize(reader);
        }
        page_id = reader.template read<uint64_t>();
    }
};

/**
 * @brief Persistent multi-value secondary index using B+Tree storage
 * @tparam T Record type
 * @tparam FieldType Type of the indexed field
 *
 * This class provides persistent secondary indexing for non-unique fields.
 * Multiple records can have the same field value.
 *
 * Unlike the old memory-based MultiValueSecondaryIndex, this version:
 * - Persists to disk using B+Tree storage
 * - Survives database restarts
 * - Supports efficient range queries
 * - Uses composite keys to handle duplicates
 *
 * For unique secondary indexes, use PersistentSecondaryIndex.
 *
 * Example:
 * @code
 * auto storage = std::make_shared<StorageEngine>("db.db");
 *
 * // Create multi-value index on department field (non-unique)
 * PersistentMultiValueSecondaryIndex<Employee, std::string> dept_index(
 *     "department",
 *     [](const Employee& e) { return e.get_department(); },
 *     storage
 * );
 *
 * // Insert multiple employees in same department
 * dept_index.insert(alice, RecordId{1, 0});  // dept = "Engineering"
 * dept_index.insert(bob, RecordId{2, 0});    // dept = "Engineering"
 *
 * // Find all employees in Engineering
 * auto rids = dept_index.find("Engineering");  // Returns 2 RecordIds
 *
 * // Persist to disk
 * dept_index.flush();
 * @endcode
 */
template<typename T, typename FieldType>
class PersistentMultiValueSecondaryIndex {
public:
    using getter_type = std::function<FieldType(const T&)>;
    using composite_key_type = CompositeKey<FieldType>;
    using index_type = PersistentBTreeIndex<composite_key_type, core::RecordId>;

    /**
     * @brief Constructs a persistent multi-value secondary index
     * @param field_name Name of the indexed field
     * @param getter Function to extract field value from record
     * @param storage Shared pointer to storage engine
     * @param root_page_id Root page ID (0 for new index)
     */
    PersistentMultiValueSecondaryIndex(
        std::string field_name,
        getter_type getter,
        std::shared_ptr<storage::StorageEngine> storage,
        uint64_t root_page_id = 0
    ) : field_name_(std::move(field_name)),
        getter_(std::move(getter)),
        index_(std::make_unique<index_type>(storage, root_page_id)) {
    }

    /**
     * @brief Inserts a record into the index
     * @param record The record to index
     * @param rid RecordId of the record
     * @return true if inserted successfully
     *
     * Uses composite key (field_value, page_id) to ensure uniqueness.
     */
    bool insert(const T& record, const core::RecordId& rid) {
        FieldType field_value = getter_(record);
        composite_key_type key(field_value, rid.page_id);
        return index_->insert(key, rid);
    }

    /**
     * @brief Finds all records with a given field value
     * @param value Field value to search for
     * @return Vector of RecordIds with matching field value
     *
     * Uses B+Tree range query to find all composite keys with this field value.
     */
    [[nodiscard]] std::vector<core::RecordId> find(const FieldType& value) const {
        // Create range: (value, 0) to (value, MAX_UINT64)
        composite_key_type min_key(value, 0);
        composite_key_type max_key(value, std::numeric_limits<uint64_t>::max());

        return index_->range_query(min_key, max_key);
    }

    /**
     * @brief Checks if any record has the given field value
     * @param value Field value to check
     * @return true if at least one record has this value
     */
    [[nodiscard]] bool contains(const FieldType& value) const {
        auto results = find(value);
        return !results.empty();
    }

    /**
     * @brief Counts how many records have the given field value
     * @param value Field value to count
     * @return Number of records with this value
     */
    [[nodiscard]] std::size_t count(const FieldType& value) const {
        auto results = find(value);
        return results.size();
    }

    /**
     * @brief Removes a specific record from the index
     * @param record The record to remove
     * @param rid RecordId of the record
     * @return true if removed, false if not found
     */
    bool remove(const T& record, const core::RecordId& rid) {
        FieldType field_value = getter_(record);
        composite_key_type key(field_value, rid.page_id);
        return index_->remove(key);
    }

    /**
     * @brief Removes all records with a given field value
     * @param value Field value to remove
     * @return Number of records removed
     */
    std::size_t remove_all(const FieldType& value) {
        auto rids = find(value);
        std::size_t removed_count = 0;

        for (const auto& rid : rids) {
            composite_key_type key(value, rid.page_id);
            if (index_->remove(key)) {
                ++removed_count;
            }
        }

        return removed_count;
    }

    /**
     * @brief Updates the index when a record is modified
     * @param old_record The old record state
     * @param new_record The new record state
     * @param rid RecordId of the record
     * @return true if updated successfully
     *
     * Handles the case where the indexed field value changes.
     */
    bool update(const T& old_record, const T& new_record, const core::RecordId& rid) {
        FieldType old_value = getter_(old_record);
        FieldType new_value = getter_(new_record);

        // If field value hasn't changed, nothing to do
        if (old_value == new_value) {
            return true;
        }

        // Remove old entry
        composite_key_type old_key(old_value, rid.page_id);
        if (!index_->remove(old_key)) {
            return false;
        }

        // Insert new entry
        composite_key_type new_key(new_value, rid.page_id);
        return index_->insert(new_key, rid);
    }

    /**
     * @brief Gets all unique field values
     * @return Vector of unique field values in sorted order
     *
     * This scans all entries and extracts unique field values.
     */
    [[nodiscard]] std::vector<FieldType> get_unique_values() const {
        std::vector<FieldType> unique_values;
        auto all_entries = index_->get_all();

        if (all_entries.empty()) {
            return unique_values;
        }

        // Extract unique values (entries are sorted by composite key)
        FieldType last_value = all_entries[0].first.field_value;
        unique_values.push_back(last_value);

        for (std::size_t i = 1; i < all_entries.size(); ++i) {
            if (all_entries[i].first.field_value != last_value) {
                last_value = all_entries[i].first.field_value;
                unique_values.push_back(last_value);
            }
        }

        return unique_values;
    }

    /**
     * @brief Gets all indexed values and their RecordIds
     * @return Vector of (field_value, RecordId) pairs
     */
    [[nodiscard]] std::vector<std::pair<FieldType, core::RecordId>> get_all() const {
        auto all_entries = index_->get_all();
        std::vector<std::pair<FieldType, core::RecordId>> results;
        results.reserve(all_entries.size());

        for (const auto& [composite_key, rid] : all_entries) {
            results.emplace_back(composite_key.field_value, rid);
        }

        return results;
    }

    /**
     * @brief Gets the total number of entries in the index
     */
    [[nodiscard]] std::size_t size() const noexcept {
        return index_->size();
    }

    /**
     * @brief Checks if the index is empty
     */
    [[nodiscard]] bool empty() const noexcept {
        return index_->empty();
    }

    /**
     * @brief Gets the field name
     */
    [[nodiscard]] const std::string& get_field_name() const noexcept {
        return field_name_;
    }

    /**
     * @brief Gets the root page ID for persistence
     * @return Root page ID of the underlying B+Tree
     */
    [[nodiscard]] uint64_t get_root_page_id() const noexcept {
        return index_->get_root_page_id();
    }

    /**
     * @brief Flushes all changes to disk
     */
    void flush() {
        index_->flush();
    }

    /**
     * @brief Clears the node cache and flushes
     */
    void clear_cache() {
        index_->clear_cache();
    }

    /**
     * @brief Creates a batch iterator for the index
     * @tparam BatchSize Number of entries per batch
     * @return BatchIterator for efficient traversal
     *
     * Uses B+Tree leaf linking for optimal performance.
     */
    template<std::size_t BatchSize = 10>
    [[nodiscard]] auto create_batch_iterator() const {
        return index_->template create_batch_iterator<BatchSize>();
    }

private:
    std::string field_name_;          ///< Name of the indexed field
    getter_type getter_;              ///< Function to extract field value
    std::unique_ptr<index_type> index_; ///< Underlying B+Tree index
};

} // namespace learnql::index

#endif // LEARNQL_INDEX_PERSISTENT_MULTI_VALUE_SECONDARY_INDEX_HPP
