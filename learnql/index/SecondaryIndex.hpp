#ifndef LEARNQL_INDEX_SECONDARY_INDEX_HPP
#define LEARNQL_INDEX_SECONDARY_INDEX_HPP

#include "BTreeIndex.hpp"
#include "../core/RecordId.hpp"
#include <string>
#include <functional>
#include <vector>
#include <memory>

namespace learnql::index {

/**
 * @brief Secondary index for non-primary-key fields
 * @tparam T Record type
 * @tparam FieldType Type of the indexed field
 *
 * Allows efficient querying by secondary fields.
 *
 * Example:
 * @code
 * SecondaryIndex<Student, std::string> name_index("name", &Student::get_name);
 * name_index.insert(student, RecordId{1, 0});
 * auto rids = name_index.find("Alice");
 * @endcode
 */
template<typename T, typename FieldType>
class SecondaryIndex {
public:
    using getter_type = std::function<FieldType(const T&)>;
    using index_type = BTreeIndex<FieldType, core::RecordId>;

    /**
     * @brief Constructs a secondary index
     * @param field_name Name of the field being indexed
     * @param getter Function to extract the field value
     */
    SecondaryIndex(std::string field_name, getter_type getter)
        : field_name_(std::move(field_name)),
          getter_(std::move(getter)),
          index_(std::make_unique<index_type>()) {}

    /**
     * @brief Inserts a record into the index
     * @param record The record
     * @param rid Record ID
     */
    void insert(const T& record, const core::RecordId& rid) {
        auto field_value = getter_(record);
        index_->insert(field_value, rid);
    }

    /**
     * @brief Finds all RecordIds for a given field value
     * @param value The field value to search for
     * @return Vector of RecordIds (may be empty if not found)
     *
     * Note: This returns a vector because secondary indexes may not be unique
     */
    [[nodiscard]] std::vector<core::RecordId> find(const FieldType& value) const {
        auto result = index_->find(value);
        if (result.has_value()) {
            return {*result};
        }
        return {};
    }

    /**
     * @brief Range query on the indexed field
     * @param min_value Minimum value (inclusive)
     * @param max_value Maximum value (inclusive)
     * @return Vector of RecordIds in range
     */
    [[nodiscard]] std::vector<core::RecordId> range_query(
        const FieldType& min_value,
        const FieldType& max_value) const {
        return index_->range_query(min_value, max_value);
    }

    /**
     * @brief Removes a record from the index
     * @param record The record
     * @return true if removed
     */
    bool remove(const T& record) {
        auto field_value = getter_(record);
        return index_->remove(field_value);
    }

    /**
     * @brief Updates the index when a record changes
     * @param old_record The old record
     * @param new_record The new record
     * @param rid Record ID
     */
    void update(const T& old_record, const T& new_record, const core::RecordId& rid) {
        auto old_value = getter_(old_record);
        auto new_value = getter_(new_record);

        if (old_value != new_value) {
            index_->remove(old_value);
            index_->insert(new_value, rid);
        }
    }

    /**
     * @brief Gets the field name
     */
    [[nodiscard]] const std::string& field_name() const noexcept {
        return field_name_;
    }

    /**
     * @brief Gets the number of entries in the index
     */
    [[nodiscard]] std::size_t size() const noexcept {
        return index_->size();
    }

    /**
     * @brief Clears the index
     */
    void clear() {
        index_->clear();
    }

private:
    std::string field_name_;
    getter_type getter_;
    std::unique_ptr<index_type> index_;
};

/**
 * @brief Multi-value secondary index for non-unique fields
 * @tparam T Record type
 * @tparam FieldType Type of the indexed field
 *
 * Unlike SecondaryIndex, this supports multiple records with the same field value.
 */
template<typename T, typename FieldType>
class MultiValueSecondaryIndex {
public:
    using getter_type = std::function<FieldType(const T&)>;
    using map_type = std::unordered_map<FieldType, std::vector<core::RecordId>>;

    /**
     * @brief Constructs a multi-value secondary index
     * @param field_name Name of the field being indexed
     * @param getter Function to extract the field value
     */
    MultiValueSecondaryIndex(std::string field_name, getter_type getter)
        : field_name_(std::move(field_name)),
          getter_(std::move(getter)) {}

    /**
     * @brief Inserts a record into the index
     * @param record The record
     * @param rid Record ID
     */
    void insert(const T& record, const core::RecordId& rid) {
        auto field_value = getter_(record);
        index_[field_value].push_back(rid);
    }

    /**
     * @brief Finds all RecordIds for a given field value
     * @param value The field value to search for
     * @return Vector of RecordIds (may be empty if not found)
     */
    [[nodiscard]] std::vector<core::RecordId> find(const FieldType& value) const {
        auto it = index_.find(value);
        if (it != index_.end()) {
            return it->second;
        }
        return {};
    }

    /**
     * @brief Removes a specific record from the index
     * @param record The record
     * @param rid Record ID
     * @return true if removed
     */
    bool remove(const T& record, const core::RecordId& rid) {
        auto field_value = getter_(record);
        auto it = index_.find(field_value);

        if (it != index_.end()) {
            auto& rids = it->second;
            auto rid_it = std::find(rids.begin(), rids.end(), rid);

            if (rid_it != rids.end()) {
                rids.erase(rid_it);

                // Remove the key if no more values
                if (rids.empty()) {
                    index_.erase(it);
                }

                return true;
            }
        }

        return false;
    }

    /**
     * @brief Updates the index when a record changes
     * @param old_record The old record
     * @param new_record The new record
     * @param rid Record ID
     */
    void update(const T& old_record, const T& new_record, const core::RecordId& rid) {
        auto old_value = getter_(old_record);
        auto new_value = getter_(new_record);

        if (old_value != new_value) {
            remove(old_record, rid);
            insert(new_record, rid);
        }
    }

    /**
     * @brief Gets the field name
     */
    [[nodiscard]] const std::string& field_name() const noexcept {
        return field_name_;
    }

    /**
     * @brief Gets the number of unique values in the index
     */
    [[nodiscard]] std::size_t size() const noexcept {
        return index_.size();
    }

    /**
     * @brief Gets the total number of indexed records
     */
    [[nodiscard]] std::size_t total_records() const noexcept {
        std::size_t count = 0;
        for (const auto& [_, rids] : index_) {
            count += rids.size();
        }
        return count;
    }

    /**
     * @brief Clears the index
     */
    void clear() {
        index_.clear();
    }

private:
    std::string field_name_;
    getter_type getter_;
    map_type index_;
};

} // namespace learnql::index

#endif // LEARNQL_INDEX_SECONDARY_INDEX_HPP
