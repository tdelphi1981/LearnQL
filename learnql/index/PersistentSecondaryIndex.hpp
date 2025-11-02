#ifndef LEARNQL_INDEX_PERSISTENT_SECONDARY_INDEX_HPP
#define LEARNQL_INDEX_PERSISTENT_SECONDARY_INDEX_HPP

#include "PersistentBTreeIndex.hpp"
#include "../core/RecordId.hpp"
#include "../storage/StorageEngine.hpp"
#include <functional>
#include <vector>
#include <string>
#include <optional>
#include <memory>

namespace learnql::index {

/**
 * @brief Persistent secondary index using B+Tree storage
 * @tparam T Record type
 * @tparam FieldType Type of the indexed field
 *
 * This class provides persistent secondary indexing on a single field.
 * Unlike the old memory-based SecondaryIndex, this version:
 * - Persists to disk using B+Tree storage
 * - Survives database restarts
 * - Supports efficient range queries via B+Tree leaf linking
 * - Uses the same optimized PersistentBTreeIndex as primary keys
 *
 * For unique secondary indexes (one record per field value).
 * For non-unique fields, use PersistentMultiValueSecondaryIndex.
 *
 * Example:
 * @code
 * auto storage = std::make_shared<StorageEngine>("db.db");
 *
 * // Create secondary index on email field
 * PersistentSecondaryIndex<User, std::string> email_index(
 *     "email",
 *     [](const User& u) { return u.get_email(); },
 *     storage
 * );
 *
 * // Insert
 * email_index.insert(user, RecordId{1, 0});
 *
 * // Lookup by email
 * auto rid = email_index.find("alice@example.com");
 *
 * // Persist to disk
 * email_index.flush();
 * @endcode
 */
template<typename T, typename FieldType>
class PersistentSecondaryIndex {
public:
    using getter_type = std::function<FieldType(const T&)>;
    using index_type = PersistentBTreeIndex<FieldType, core::RecordId>;

    /**
     * @brief Constructs a persistent secondary index
     * @param field_name Name of the indexed field
     * @param getter Function to extract field value from record
     * @param storage Shared pointer to storage engine
     * @param root_page_id Root page ID (0 for new index)
     */
    PersistentSecondaryIndex(
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
     * @return true if inserted, false if field value already exists
     */
    bool insert(const T& record, const core::RecordId& rid) {
        FieldType field_value = getter_(record);
        return index_->insert(field_value, rid);
    }

    /**
     * @brief Finds a record by field value
     * @param value Field value to search for
     * @return Optional RecordId if found
     */
    [[nodiscard]] std::optional<core::RecordId> find(const FieldType& value) const {
        return index_->find(value);
    }

    /**
     * @brief Checks if a field value exists
     * @param value Field value to check
     * @return true if value exists in index
     */
    [[nodiscard]] bool contains(const FieldType& value) const {
        return index_->contains(value);
    }

    /**
     * @brief Removes a record from the index
     * @param record The record to remove
     * @return true if removed, false if not found
     */
    bool remove(const T& record) {
        FieldType field_value = getter_(record);
        return index_->remove(field_value);
    }

    /**
     * @brief Removes a record by field value
     * @param value Field value to remove
     * @return true if removed, false if not found
     */
    bool remove_by_value(const FieldType& value) {
        return index_->remove(value);
    }

    /**
     * @brief Updates the index when a record is modified
     * @param old_record The old record state
     * @param new_record The new record state
     * @param rid RecordId of the record
     * @return true if updated successfully
     *
     * This handles the case where the indexed field value changes.
     */
    bool update(const T& old_record, const T& new_record, const core::RecordId& rid) {
        FieldType old_value = getter_(old_record);
        FieldType new_value = getter_(new_record);

        // If field value hasn't changed, nothing to do
        if (old_value == new_value) {
            return true;
        }

        // Remove old entry
        if (!index_->remove(old_value)) {
            return false;
        }

        // Insert new entry
        return index_->insert(new_value, rid);
    }

    /**
     * @brief Performs a range query on the index
     * @param min_value Minimum field value (inclusive)
     * @param max_value Maximum field value (inclusive)
     * @return Vector of RecordIds in range
     *
     * Leverages B+Tree leaf linking for efficient range scans.
     */
    [[nodiscard]] std::vector<core::RecordId> range_query(
        const FieldType& min_value,
        const FieldType& max_value
    ) const {
        return index_->range_query(min_value, max_value);
    }

    /**
     * @brief Gets all indexed values and their RecordIds
     * @return Vector of (field_value, RecordId) pairs
     */
    [[nodiscard]] std::vector<std::pair<FieldType, core::RecordId>> get_all() const {
        return index_->get_all();
    }

    /**
     * @brief Gets the number of entries in the index
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

#endif // LEARNQL_INDEX_PERSISTENT_SECONDARY_INDEX_HPP
