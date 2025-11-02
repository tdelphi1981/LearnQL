#ifndef LEARNQL_CORE_TABLE_HPP
#define LEARNQL_CORE_TABLE_HPP

#include "RecordId.hpp"
#include "../concepts/Queryable.hpp"
#include "../storage/StorageEngine.hpp"
#include "../serialization/BinaryWriter.hpp"
#include "../serialization/BinaryReader.hpp"
#include "../ranges/QueryView.hpp"
#include "../ranges/ProxyVector.hpp"
#include "../index/PersistentBTreeIndex.hpp"
#include "../index/PersistentSecondaryIndex.hpp"
#include "../index/PersistentMultiValueSecondaryIndex.hpp"
#include "../query/Field.hpp"
#include <vector>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <functional>

namespace learnql {
    // Forward declarations
    namespace catalog {
        class SystemCatalog;
    }
}

namespace learnql::core {

/**
 * @brief Secondary index type specification
 */
enum class IndexType {
    Unique,      ///< One record per field value (uses PersistentSecondaryIndex)
    MultiValue   ///< Multiple records per field value (uses PersistentMultiValueSecondaryIndex)
};

/**
 * @brief Type-safe table for storing objects of type T
 * @tparam T Type of objects to store (must satisfy Queryable concept)
 * @tparam BatchSize Number of record IDs to load per batch (default: 10)
 *
 * Features:
 * - Compile-time type checking via concepts
 * - CRUD operations: insert, update, remove, find
 * - Primary key-based indexing
 * - Secondary indexes with automatic synchronization (NEW!)
 * - Iterator support for range-based loops
 * - Integration with storage engine
 * - Lazy batch loading (records loaded in configurable batches)
 * - Memory-efficient traversal
 *
 * Example:
 * @code
 * Table<Student> students(storage_engine, "students");
 * students.add_index(Student::email, IndexType::Unique);
 * students.insert(alice);
 * auto student = students.find_by(Student::email, "alice@uni.edu");
 * for (const auto& s : students) {
 *     std::cout << s.get_name() << "\n";
 * }
 * @endcode
 */
template<typename T, std::size_t BatchSize = 10>
requires concepts::Queryable<T, serialization::BinaryWriter, serialization::BinaryReader>
class Table {
public:
    using value_type = T;
    using primary_key_type = typename T::primary_key_type;
    using index_type = index::PersistentBTreeIndex<primary_key_type, RecordId>;

    /**
     * @brief Forward iterator for table with batched loading
     *
     * This iterator loads record IDs in batches rather than caching all
     * IDs upfront, making iteration more memory-efficient for large tables.
     */
    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;
        using batch_iterator_type = index::BatchIterator<primary_key_type, RecordId, BatchSize>;

    private:
        const Table* table_;
        mutable std::shared_ptr<batch_iterator_type> batch_iter_;
        mutable std::vector<std::pair<primary_key_type, RecordId>> current_batch_;
        mutable std::size_t batch_index_;
        mutable T current_;
        mutable bool current_loaded_;
        bool is_end_;

        void load_current() const {
            if (is_end_ || batch_index_ >= current_batch_.size()) {
                current_loaded_ = false;
                return;
            }

            try {
                current_ = table_->load_record(current_batch_[batch_index_].second);
                current_loaded_ = true;
            } catch (const std::exception&) {
                current_loaded_ = false;
                throw;
            }
        }

        void load_next_batch() const {
            if (!batch_iter_ || !batch_iter_->has_more()) {
                current_batch_.clear();
                return;
            }

            current_batch_ = batch_iter_->next_batch();
            batch_index_ = 0;
        }

    public:
        // Constructor for begin iterator
        Iterator(const Table* table, std::shared_ptr<batch_iterator_type> batch_iter, bool is_end)
            : table_(table),
              batch_iter_(batch_iter),
              current_batch_(),
              batch_index_(0),
              current_{},
              current_loaded_(false),
              is_end_(is_end) {

            if (!is_end_ && batch_iter_) {
                load_next_batch();
                if (!current_batch_.empty()) {
                    load_current();
                } else {
                    is_end_ = true;
                }
            }
        }

        reference operator*() const {
            if (!current_loaded_) {
                throw std::runtime_error("Dereferencing invalid iterator");
            }
            return current_;
        }

        pointer operator->() const {
            return &(**this);
        }

        Iterator& operator++() {
            if (is_end_) {
                return *this;
            }

            ++batch_index_;

            // Check if we need to load the next batch
            if (batch_index_ >= current_batch_.size()) {
                load_next_batch();

                // Check if we've reached the end
                if (current_batch_.empty()) {
                    is_end_ = true;
                    current_loaded_ = false;
                    return *this;
                }
            }

            load_current();
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const Iterator& other) const {
            // Both are end iterators
            if (is_end_ && other.is_end_) {
                return true;
            }
            // One is end, the other is not
            if (is_end_ != other.is_end_) {
                return false;
            }
            // Both are valid iterators - compare by batch iterator
            return batch_iter_ == other.batch_iter_ && batch_index_ == other.batch_index_;
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }
    };

    /**
     * @brief Constructs a table
     * @param storage Shared pointer to storage engine
     * @param table_name Name of the table
     * @param root_page_id Root page ID for the B-tree index (0 for new table)
     */
    Table(std::shared_ptr<storage::StorageEngine> storage,
          std::string table_name,
          uint64_t root_page_id = 0)
        : storage_(storage),
          table_name_(std::move(table_name)),
          index_(nullptr),
          count_(0),
          catalog_(nullptr) {
        // Create or load index
        index_ = std::make_unique<index_type>(storage_, root_page_id);

        // Count entries in index
        count_ = index_->size();
    }

    /**
     * @brief Destructor - flushes index
     */
    ~Table() {
        try {
            if (index_) {
                index_->flush();
            }
        } catch (...) {
            // Suppress exceptions in destructor
        }
    }

    /**
     * @brief Inserts a new record
     * @param record Record to insert
     * @throws std::runtime_error if a record with this primary key already exists
     */
    void insert(const T& record) {
        auto key = record.get_primary_key();

        // Check if record already exists
        if (index_->contains(key)) {
            throw std::runtime_error("Record with primary key already exists");
        }

        // Serialize the record
        serialization::BinaryWriter writer;
        writer.write(record);
        auto data = writer.get_buffer();

        // Allocate a page if needed
        uint64_t page_id = storage_->allocate_page(storage::PageType::DATA);

        // Write to page
        auto page = storage_->read_page(page_id);
        if (!page.can_fit(data.size())) {
            throw std::runtime_error("Record too large for single page");
        }

        page.write_data(0, data.data(), data.size());
        page.header().record_count = 1;
        page.header().free_space_offset = sizeof(storage::PageHeader) + data.size();
        storage_->write_page(page_id, page);

        // Update primary index
        RecordId rid{page_id, 0};
        index_->insert(key, rid);

        // Update all secondary indexes
        for (auto& sec_idx : secondary_indexes_) {
            sec_idx->insert_record(record, rid);
        }

        ++count_;

        // Notify catalog of count change
        sync_catalog_count();
    }

    /**
     * @brief Updates an existing record
     * @param record Record with updated data
     * @throws std::runtime_error if record not found
     */
    void update(const T& record) {
        auto key = record.get_primary_key();

        auto rid_opt = index_->find(key);
        if (!rid_opt) {
            throw std::runtime_error("Record not found");
        }

        // Load the old record for secondary index updates
        T old_record = load_record(*rid_opt);

        // Serialize the updated record
        serialization::BinaryWriter writer;
        writer.write(record);
        auto data = writer.get_buffer();

        // Read the page
        auto page = storage_->read_page(rid_opt->page_id);

        // Check if it fits (simple implementation - no page splitting yet)
        if (!page.can_fit(data.size())) {
            throw std::runtime_error("Updated record too large");
        }

        // Write updated data
        page.write_data(0, data.data(), data.size());
        storage_->write_page(rid_opt->page_id, page);

        // Update all secondary indexes
        for (auto& sec_idx : secondary_indexes_) {
            sec_idx->update_record(old_record, record, *rid_opt);
        }
    }

    /**
     * @brief Removes a record by primary key
     * @param key Primary key of record to remove
     * @return true if record was removed, false if not found
     */
    bool remove(const primary_key_type& key) {
        auto rid_opt = index_->find(key);
        if (!rid_opt) {
            return false;
        }

        // Load the record for secondary index removal
        T record = load_record(*rid_opt);

        // Remove from all secondary indexes first
        for (auto& sec_idx : secondary_indexes_) {
            sec_idx->remove_record(record, *rid_opt);
        }

        // Deallocate the page
        storage_->deallocate_page(rid_opt->page_id);

        // Remove from primary index
        index_->remove(key);
        --count_;

        // Notify catalog of count change
        sync_catalog_count();

        return true;
    }

    /**
     * @brief Finds a record by primary key
     * @param key Primary key to search for
     * @return Pointer to record, or nullptr if not found
     */
    [[nodiscard]] std::unique_ptr<T> find(const primary_key_type& key) const {
        auto rid_opt = index_->find(key);
        if (!rid_opt) {
            return nullptr;
        }

        try {
            return std::make_unique<T>(load_record(*rid_opt));
        } catch (const std::exception&) {
            return nullptr;
        }
    }

    /**
     * @brief Checks if a record exists
     * @param key Primary key to check
     * @return true if record exists
     */
    [[nodiscard]] bool contains(const primary_key_type& key) const {
        return index_->contains(key);
    }

    /**
     * @brief Gets the RecordId for a given primary key
     * @param key Primary key to look up
     * @return Optional RecordId if the key exists
     *
     * This is useful for:
     * - Populating secondary indexes from existing data
     * - Direct page-level operations
     * - Custom index implementations
     *
     * Example:
     * @code
     * auto rid = students.get_record_id(1001);
     * if (rid) {
     *     secondary_index.insert(student, *rid);
     * }
     * @endcode
     */
    [[nodiscard]] std::optional<RecordId> get_record_id(const primary_key_type& key) const {
        return index_->find(key);
    }

    // ========================================================================
    // Secondary Index Management (NEW!)
    // ========================================================================

    /**
     * @brief Adds a secondary index on a field
     * @tparam FieldType Type of the indexed field
     * @param field Static Field object (auto-generated by LEARNQL_PROPERTY)
     * @param type Index type (Unique or MultiValue)
     * @return Reference to this table for method chaining
     *
     * Creates a persistent secondary index on the specified field.
     * The index is automatically synchronized on all CRUD operations.
     *
     * Example:
     * @code
     * auto& students = db.table<Student>("students")
     *     .add_index(Student::email, IndexType::Unique)
     *     .add_index(Student::department, IndexType::MultiValue);
     *
     * // Now you can use find_by():
     * auto alice = students.find_by(Student::email, "alice@uni.edu");
     * @endcode
     */
    template<typename FieldType>
    Table& add_index(const query::Field<T, FieldType>& field, IndexType type);

    /**
     * @brief Removes a secondary index on a field
     * @tparam FieldType Type of the indexed field
     * @param field Static Field object
     * @return true if index was found and removed
     *
     * Example:
     * @code
     * students.drop_index(Student::email);
     * @endcode
     */
    template<typename FieldType>
    bool drop_index(const query::Field<T, FieldType>& field);

    // ========================================================================
    // Secondary Index Queries (NEW!)
    // ========================================================================

    /**
     * @brief Finds a single record by indexed field value (unique index)
     * @tparam FieldType Type of the indexed field
     * @param field Static Field object
     * @param value Value to search for
     * @return Optional record if found
     *
     * This method uses a unique secondary index for O(log n) lookup.
     *
     * Example:
     * @code
     * auto alice = students.find_by(Student::email, "alice@uni.edu");
     * if (alice) {
     *     std::cout << alice->get_name() << "\n";
     * }
     * @endcode
     */
    template<typename FieldType>
    [[nodiscard]] std::optional<T> find_by(const query::Field<T, FieldType>& field, const FieldType& value) const {
        // Find the secondary index
        const std::string& field_name = field.expr().name();
        for (const auto& sec_idx : secondary_indexes_) {
            if (sec_idx->get_field_name() == field_name && sec_idx->is_unique()) {
                // Cast to typed wrapper to access find_unique
                auto* wrapper = dynamic_cast<SecondaryIndexWrapper<FieldType>*>(sec_idx.get());
                if (wrapper) {
                    auto rid_opt = wrapper->find_unique(value);
                    if (rid_opt) {
                        try {
                            return load_record(*rid_opt);
                        } catch (const std::exception&) {
                            return std::nullopt;
                        }
                    }
                }
            }
        }
        return std::nullopt;
    }

    /**
     * @brief Finds all records by indexed field value (multi-value index)
     * @tparam FieldType Type of the indexed field
     * @param field Static Field object
     * @param value Value to search for
     * @return Vector of matching records
     *
     * This method uses a multi-value secondary index for O(log n) lookup.
     *
     * Example:
     * @code
     * auto cs_students = students.find_all_by(Student::department, "CS");
     * for (const auto& student : cs_students) {
     *     std::cout << student.get_name() << "\n";
     * }
     * @endcode
     */
    template<typename FieldType>
    [[nodiscard]] std::vector<T> find_all_by(const query::Field<T, FieldType>& field, const FieldType& value) const {
        std::vector<T> results;

        // Find the secondary index
        const std::string& field_name = field.expr().name();
        for (const auto& sec_idx : secondary_indexes_) {
            if (sec_idx->get_field_name() == field_name) {
                // Cast to typed wrapper to access find_multi
                auto* wrapper = dynamic_cast<SecondaryIndexWrapper<FieldType>*>(sec_idx.get());
                if (wrapper) {
                    auto rids = wrapper->find_multi(value);
                    results.reserve(rids.size());

                    for (const auto& rid : rids) {
                        try {
                            results.push_back(load_record(rid));
                        } catch (const std::exception&) {
                            // Skip corrupted records
                            continue;
                        }
                    }
                    break;
                }
            }
        }

        return results;
    }

    /**
     * @brief Gets all unique values from a multi-value indexed field
     * @tparam FieldType Type of the indexed field
     * @param field Static Field object
     * @return Vector of unique field values
     *
     * Example:
     * @code
     * auto departments = students.get_unique_values(Student::department);
     * for (const auto& dept : departments) {
     *     std::cout << dept << "\n";
     * }
     * @endcode
     */
    template<typename FieldType>
    [[nodiscard]] std::vector<FieldType> get_unique_values(const query::Field<T, FieldType>& field) const {
        // Find the secondary index
        const std::string& field_name = field.expr().name();
        for (const auto& sec_idx : secondary_indexes_) {
            if (sec_idx->get_field_name() == field_name && !sec_idx->is_unique()) {
                // Cast to typed wrapper to access get_unique_values
                auto* wrapper = dynamic_cast<SecondaryIndexWrapper<FieldType>*>(sec_idx.get());
                if (wrapper) {
                    return wrapper->get_unique_values();
                }
            }
        }
        return {};
    }

    /**
     * @brief Performs a range query on an indexed field
     * @tparam FieldType Type of the indexed field
     * @param field Static Field object
     * @param min_value Minimum value (inclusive)
     * @param max_value Maximum value (exclusive)
     * @return Vector of matching records
     *
     * Example:
     * @code
     * auto names_a_m = students.range_query(Student::name, "A", "M");
     * @endcode
     */
    template<typename FieldType>
    [[nodiscard]] std::vector<T> range_query(
        const query::Field<T, FieldType>& field,
        const FieldType& min_value,
        const FieldType& max_value
    ) const {
        std::vector<T> results;

        // Find the secondary index
        const std::string& field_name = field.expr().name();
        for (const auto& sec_idx : secondary_indexes_) {
            if (sec_idx->get_field_name() == field_name && sec_idx->is_unique()) {
                // Cast to typed wrapper to access range_query
                auto* wrapper = dynamic_cast<SecondaryIndexWrapper<FieldType>*>(sec_idx.get());
                if (wrapper) {
                    auto rids = wrapper->range_query(min_value, max_value);
                    results.reserve(rids.size());

                    for (const auto& rid : rids) {
                        try {
                            results.push_back(load_record(rid));
                        } catch (const std::exception&) {
                            // Skip corrupted records
                            continue;
                        }
                    }
                    break;
                }
            }
        }

        return results;
    }

    /**
     * @brief Gets the number of records in the table
     */
    [[nodiscard]] std::size_t size() const noexcept {
        return count_;
    }

    /**
     * @brief Checks if the table is empty
     */
    [[nodiscard]] bool empty() const noexcept {
        return count_ == 0;
    }

    /**
     * @brief Clears all records from the table
     */
    void clear() {
        // Use batch iterator to deallocate pages
        auto batch_iter = index_->template create_batch_iterator<BatchSize>();

        while (batch_iter.has_more()) {
            auto batch = batch_iter.next_batch();
            for (const auto& [key, rid] : batch) {
                storage_->deallocate_page(rid.page_id);
            }
        }

        // Clear the index by creating a new one
        index_ = std::make_unique<index_type>(storage_, 0);
        count_ = 0;

        // Notify catalog of count change
        sync_catalog_count();
    }

    /**
     * @brief Finds all records matching a predicate
     * @tparam Predicate Function type (bool(const T&))
     * @param pred Predicate function
     * @return ProxyVector of matching records (loaded in batches)
     */
    template<concepts::Predicate<T> Predicate>
    [[nodiscard]] ranges::ProxyVector<T, BatchSize> find_if(Predicate pred) const {
        // Create a batch iterator for the index
        auto batch_iter = index_->template create_batch_iterator<BatchSize>();

        // Create a fetcher that loads the next batch of matching records
        auto fetcher = [this, pred, iter = std::make_shared<decltype(batch_iter)>(std::move(batch_iter))]() mutable -> std::vector<T> {
            std::vector<T> batch_results;
            batch_results.reserve(BatchSize);

            // Keep fetching batches until we have enough matching records or run out of data
            while (batch_results.size() < BatchSize && iter->has_more()) {
                auto batch = iter->next_batch();

                for (const auto& [key, rid] : batch) {
                    try {
                        T record = this->load_record(rid);
                        if (pred(record)) {
                            batch_results.push_back(std::move(record));
                            if (batch_results.size() >= BatchSize) {
                                break;
                            }
                        }
                    } catch (const std::exception&) {
                        // Skip corrupted records
                        continue;
                    }
                }
            }

            return batch_results;
        };

        return ranges::ProxyVector<T, BatchSize>(fetcher);
    }

    /**
     * @brief Gets all records
     * @return ProxyVector of all records (loaded in batches)
     */
    [[nodiscard]] ranges::ProxyVector<T, BatchSize> get_all() const {
        return find_if([](const T&) { return true; });
    }

    /**
     * @brief Creates a query builder for this table (for Phase 3)
     * @return Query builder
     * @note Forward declaration - implementation requires Query.hpp
     */
    template<typename ExprType>
    [[nodiscard]] auto where(const ExprType& expr) const;

    /**
     * @brief Creates a range view of all records (Phase 4)
     * @return QueryView of all records
     *
     * This enables using C++20 ranges with the table:
     * @code
     * auto names = students.view()
     *     | std::views::filter([](auto& s) { return s.age > 20; })
     *     | std::views::transform([](auto& s) { return s.name; });
     * @endcode
     */
    [[nodiscard]] auto view() const {
        return ranges::QueryView<T>(get_all());
    }

    /**
     * @brief Alias for view() - returns all records as a range
     * @return QueryView of all records
     */
    [[nodiscard]] auto all() const {
        return view();
    }

    /**
     * @brief Creates a range view of records matching an expression
     * @tparam ExprType Expression type
     * @param expr Expression to filter with
     * @return QueryView of matching records
     *
     * Example:
     * @code
     * auto results = students.view_where(age > 20)
     *     | std::views::transform([](auto& s) { return s.name; });
     * @endcode
     */
    template<typename ExprType>
    [[nodiscard]] auto view_where(const ExprType& expr) const {
        return ranges::QueryView<T>(where(expr));
    }

    /**
     * @brief Gets the table name
     */
    [[nodiscard]] const std::string& name() const noexcept {
        return table_name_;
    }

    /**
     * @brief Iterator to beginning (with batched loading)
     *
     * Returns an iterator that loads record IDs in batches,
     * making iteration memory-efficient for large tables.
     */
    [[nodiscard]] Iterator begin() const {
        auto batch_iter = std::make_shared<typename Iterator::batch_iterator_type>(
            index_->template create_batch_iterator<BatchSize>()
        );
        return Iterator(this, batch_iter, false);
    }

    /**
     * @brief Iterator to end
     */
    [[nodiscard]] Iterator end() const {
        return Iterator(this, nullptr, true);
    }

    /**
     * @brief Flushes all changes to disk
     *
     * Flushes primary index, all secondary indexes, and storage engine.
     */
    void flush() {
        if (index_) {
            index_->flush();
        }

        // Flush all secondary indexes
        for (auto& sec_idx : secondary_indexes_) {
            sec_idx->flush();
        }

        storage_->flush_all();
    }

    /**
     * @brief Gets the root page ID of the table's B-tree index
     * @return Root page ID
     */
    [[nodiscard]] uint64_t get_root_page() const {
        return index_ ? index_->get_root_page_id() : 0;
    }

    /**
     * @brief Sets the system catalog for metadata sync
     * @param catalog Pointer to system catalog (not owned)
     *
     * Called by Database after table creation to enable automatic
     * record count synchronization.
     */
    void set_catalog(catalog::SystemCatalog* catalog) {
        catalog_ = catalog;
    }

private:
    // ========================================================================
    // Secondary Index Infrastructure
    // ========================================================================

    /**
     * @brief Type-erased base class for secondary indexes
     *
     * This allows us to store secondary indexes of different field types
     * in a single container using polymorphism.
     */
    struct SecondaryIndexBase {
        virtual ~SecondaryIndexBase() = default;

        /// Insert a record into the index
        virtual void insert_record(const T& record, const RecordId& rid) = 0;

        /// Remove a record from the index
        virtual void remove_record(const T& record, const RecordId& rid) = 0;

        /// Update a record in the index
        virtual void update_record(const T& old_rec, const T& new_rec, const RecordId& rid) = 0;

        /// Flush index to disk
        virtual void flush() = 0;

        /// Get the root page ID
        virtual uint64_t get_root_page_id() const = 0;

        /// Get the field name
        virtual std::string get_field_name() const = 0;

        /// Check if this is a unique index
        virtual bool is_unique() const = 0;

        /// Get field type name
        virtual std::string get_field_type() const = 0;
    };

    /**
     * @brief Concrete wrapper for typed secondary indexes
     * @tparam FieldType Type of the indexed field
     *
     * This class wraps either a PersistentSecondaryIndex (unique) or
     * PersistentMultiValueSecondaryIndex (multi-value) and implements
     * the SecondaryIndexBase interface for type erasure.
     */
    template<typename FieldType>
    struct SecondaryIndexWrapper : SecondaryIndexBase {
        using getter_type = std::function<FieldType(const T&)>;
        using unique_index_type = index::PersistentSecondaryIndex<T, FieldType>;
        using multi_index_type = index::PersistentMultiValueSecondaryIndex<T, FieldType>;

        std::unique_ptr<unique_index_type> unique_index_;
        std::unique_ptr<multi_index_type> multi_index_;
        getter_type getter_;
        std::string field_name_;
        bool is_unique_;

        // Constructor for unique index
        SecondaryIndexWrapper(
            std::string field_name,
            getter_type getter,
            std::shared_ptr<storage::StorageEngine> storage,
            bool unique,
            uint64_t root_page_id = 0
        ) : getter_(std::move(getter)),
            field_name_(std::move(field_name)),
            is_unique_(unique) {

            if (is_unique_) {
                unique_index_ = std::make_unique<unique_index_type>(
                    field_name_,
                    getter_,
                    storage,
                    root_page_id
                );
            } else {
                multi_index_ = std::make_unique<multi_index_type>(
                    field_name_,
                    getter_,
                    storage,
                    root_page_id
                );
            }
        }

        void insert_record(const T& record, const RecordId& rid) override {
            if (is_unique_) {
                unique_index_->insert(record, rid);
            } else {
                multi_index_->insert(record, rid);
            }
        }

        void remove_record(const T& record, const RecordId& rid) override {
            if (is_unique_) {
                unique_index_->remove(record);
            } else {
                multi_index_->remove(record, rid);
            }
        }

        void update_record(const T& old_rec, const T& new_rec, const RecordId& rid) override {
            if (is_unique_) {
                unique_index_->update(old_rec, new_rec, rid);
            } else {
                multi_index_->update(old_rec, new_rec, rid);
            }
        }

        void flush() override {
            if (is_unique_) {
                unique_index_->flush();
            } else {
                multi_index_->flush();
            }
        }

        uint64_t get_root_page_id() const override {
            return is_unique_ ? unique_index_->get_root_page_id()
                              : multi_index_->get_root_page_id();
        }

        std::string get_field_name() const override {
            return field_name_;
        }

        bool is_unique() const override {
            return is_unique_;
        }

        std::string get_field_type() const override {
            return typeid(FieldType).name();
        }

        // Query methods for use by Table's public API
        std::optional<RecordId> find_unique(const FieldType& value) const {
            if (is_unique_) {
                return unique_index_->find(value);
            }
            return std::nullopt;
        }

        std::vector<RecordId> find_multi(const FieldType& value) const {
            if (!is_unique_) {
                return multi_index_->find(value);
            }
            return {};
        }

        std::vector<RecordId> range_query(const FieldType& min_val, const FieldType& max_val) const {
            if (is_unique_) {
                return unique_index_->range_query(min_val, max_val);
            } else {
                // Multi-value index doesn't have direct range_query, but we can get all
                // This is a simplified implementation
                return {};
            }
        }

        std::vector<FieldType> get_unique_values() const {
            if (!is_unique_) {
                return multi_index_->get_unique_values();
            }
            return {};
        }
    };

    /**
     * @brief Loads a record from storage
     * @param rid Record ID
     * @return Loaded record
     * @throws std::runtime_error if record cannot be loaded
     */
    [[nodiscard]] T load_record(const RecordId& rid) const {
        // Read the page
        auto page = storage_->read_page(rid.page_id);

        // Get record data (simplified - assumes one record per page for now)
        auto data_span = page.data();
        std::vector<uint8_t> data(data_span.begin(), data_span.end());

        // Deserialize
        serialization::BinaryReader reader(data);
        return reader.read_custom<T>();
    }

    /**
     * @brief Synchronize record count with system catalog
     * @details Defined after SystemCatalog include to avoid incomplete type error
     */
    void sync_catalog_count();

private:
    std::shared_ptr<storage::StorageEngine> storage_;         ///< Storage engine (shared)
    std::string table_name_;                                  ///< Table name
    std::unique_ptr<index_type> index_;                       ///< Persistent B-Tree index (primary key)
    std::vector<std::unique_ptr<SecondaryIndexBase>> secondary_indexes_;  ///< Secondary indexes
    std::size_t count_;                                       ///< Record count
    catalog::SystemCatalog* catalog_;                         ///< System catalog (not owned)
};

// Forward declaration for Query (defined in Query.hpp)
template<typename T, std::size_t BatchSize = 10>
class Query;

/**
 * @brief Implementation of Table::where() for expression templates
 * @details This needs to be defined after Query is fully defined
 * @note Include Query.hpp before using this method
 */
template<typename T, std::size_t BatchSize>
requires concepts::Queryable<T, serialization::BinaryWriter, serialization::BinaryReader>
template<typename ExprType>
auto Table<T, BatchSize>::where(const ExprType& expr) const {
    // Return ProxyVector directly by evaluating the expression
    return find_if([expr](const T& obj) {
        return expr.evaluate(obj);
    });
}

} // namespace learnql::core

// Include SystemCatalog after Table definition to avoid circular dependency
#include "../catalog/SystemCatalog.hpp"

namespace learnql::core {

/**
 * @brief Implementation of Table::sync_catalog_count()
 * @details Must be defined after SystemCatalog is fully defined
 */
template<typename T, std::size_t BatchSize>
requires concepts::Queryable<T, serialization::BinaryWriter, serialization::BinaryReader>
void Table<T, BatchSize>::sync_catalog_count() {
    if (catalog_) {
        catalog_->update_record_count(table_name_, count_);
    }
}

/**
 * @brief Implementation of Table::add_index()
 * @details Must be defined after SystemCatalog is fully defined
 */
template<typename T, std::size_t BatchSize>
requires concepts::Queryable<T, serialization::BinaryWriter, serialization::BinaryReader>
template<typename FieldType>
Table<T, BatchSize>& Table<T, BatchSize>::add_index(
    const query::Field<T, FieldType>& field,
    IndexType type
) {
    // Check if index already exists in catalog
    uint64_t existing_root_page = 0;
    bool is_new_index = true;
    bool is_unique = (type == IndexType::Unique);

    if (catalog_) {
        auto existing_indexes = catalog_->get_table_indexes(table_name_);
        for (const auto& idx_meta : existing_indexes) {
            if (idx_meta.get_field_name() == field.expr().name()) {
                // Index exists - load it instead of creating new
                existing_root_page = idx_meta.get_index_root_page();
                is_unique = idx_meta.get_is_unique();
                is_new_index = false;
                break;
            }
        }
    }

    // Create the secondary index wrapper (will load existing or create new)
    auto wrapper = std::make_unique<SecondaryIndexWrapper<FieldType>>(
        field.expr().name(),
        field.expr().getter(),
        storage_,
        is_unique,
        existing_root_page  // 0 = create new, non-zero = load existing
    );

    // Only populate if creating a new index
    if (is_new_index) {
        auto batch_iter = index_->template create_batch_iterator<BatchSize>();
        while (batch_iter.has_more()) {
            auto batch = batch_iter.next_batch();
            for (const auto& [key, rid] : batch) {
                try {
                    T record = load_record(rid);
                    wrapper->insert_record(record, rid);
                } catch (const std::exception&) {
                    // Skip corrupted records
                    continue;
                }
            }
        }

        // Register in system catalog (only for new indexes)
        if (catalog_) {
            catalog_->register_index(
                table_name_,
                field.expr().name(),
                wrapper->get_field_type(),
                wrapper->is_unique(),
                wrapper->get_root_page_id()
            );
        }
    }

    secondary_indexes_.push_back(std::move(wrapper));

    return *this;  // Enable fluent chaining
}

/**
 * @brief Implementation of Table::drop_index()
 * @details Must be defined after SystemCatalog is fully defined
 */
template<typename T, std::size_t BatchSize>
requires concepts::Queryable<T, serialization::BinaryWriter, serialization::BinaryReader>
template<typename FieldType>
bool Table<T, BatchSize>::drop_index(const query::Field<T, FieldType>& field) {
    const std::string& field_name = field.expr().name();
    auto it = std::find_if(
        secondary_indexes_.begin(),
        secondary_indexes_.end(),
        [&field_name](const auto& idx) {
            return idx->get_field_name() == field_name;
        }
    );

    if (it != secondary_indexes_.end()) {
        // Unregister from system catalog (if available)
        if (catalog_) {
            catalog_->unregister_index(table_name_, field_name);
        }

        secondary_indexes_.erase(it);
        return true;
    }

    return false;
}

} // namespace learnql::core

#endif // LEARNQL_CORE_TABLE_HPP
