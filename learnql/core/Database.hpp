#ifndef LEARNQL_CORE_DATABASE_HPP
#define LEARNQL_CORE_DATABASE_HPP

#include "Table.hpp"
#include "../storage/StorageEngine.hpp"
#include "../catalog/SystemCatalog.hpp"
#include "../catalog/TableMetadata.hpp"
#include "../catalog/FieldMetadata.hpp"
#include "../reflection/FieldExtractor.hpp"
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <string>
#include <stdexcept>
#include <ctime>

namespace learnql::core {

/**
 * @brief Database context managing multiple tables
 * @details Provides a unified interface for accessing tables and managing storage
 *
 * Features:
 * - Type-safe table access
 * - Automatic table creation
 * - Single storage engine shared across tables
 * - RAII-based resource management
 *
 * Example:
 * @code
 * Database db("school.db");
 *
 * auto& students = db.table<Student>();
 * auto& courses = db.table<Course>();
 *
 * students.insert(alice);
 * courses.insert(math101);
 *
 * db.flush();  // Write all changes to disk
 * @endcode
 */
class Database {
public:
    /**
     * @brief Creates or opens a database
     * @param file_path Path to the database file
     * @param cache_size Number of pages to cache (default: 64)
     * @throws std::runtime_error if database cannot be opened
     */
    explicit Database(const std::string& file_path, std::size_t cache_size = 64)
        : storage_(std::make_shared<storage::StorageEngine>(file_path, cache_size)),
          tables_{},
          table_names_{},
          catalog_{nullptr} {

        // Initialize system catalog
        initialize_system_catalog();
    }

    // Disable copy (database owns resources)
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // Enable move
    Database(Database&&) noexcept = default;
    Database& operator=(Database&&) noexcept = default;

    /**
     * @brief Destructor - flushes all changes
     */
    ~Database() {
        try {
            flush();
        } catch (...) {
            // Suppress exceptions in destructor
        }
    }

    /**
     * @brief Gets or creates a table for type T
     * @tparam T Type of objects to store
     * @return Reference to the table
     * @throws std::runtime_error if table cannot be created
     *
     * The table name is derived from the type name. If the table doesn't exist,
     * it is created automatically.
     *
     * Example:
     * @code
     * auto& students = db.table<Student>();  // Creates table if not exists
     * students.insert(alice);
     * @endcode
     */
    template<typename T>
    requires concepts::Queryable<T, serialization::BinaryWriter, serialization::BinaryReader>
    Table<T>& table() {
        std::type_index type_id = std::type_index(typeid(T));

        // Check if table already exists
        auto it = tables_.find(type_id);
        if (it != tables_.end()) {
            // Cast from shared_ptr<void> to Table<T>*
            return *static_cast<Table<T>*>(it->second.get());
        }

        // Create new table
        std::string table_name = get_table_name<T>();
        auto table_ptr = new Table<T>(storage_, table_name);
        Table<T>& table_ref = *table_ptr;

        // Store in maps using shared_ptr<void> with custom deleter
        tables_[type_id] = std::shared_ptr<void>(table_ptr, [](void* p) {
            delete static_cast<Table<T>*>(p);
        });
        table_names_[type_id] = table_name;

        return table_ref;
    }

    /**
     * @brief Gets a table with a custom name
     * @tparam T Type of objects to store
     * @param table_name Name for the table
     * @return Reference to the table
     *
     * Useful when you want multiple tables of the same type with different names.
     *
     * Example:
     * @code
     * auto& active_students = db.table<Student>("active_students");
     * auto& alumni = db.table<Student>("alumni");
     * @endcode
     */
    template<typename T>
    requires concepts::Queryable<T, serialization::BinaryWriter, serialization::BinaryReader>
    Table<T>& table(const std::string& table_name) {
        // Create a unique key combining type and name
        // Use hash of the combined string to create a unique identifier
        std::string unique_key = std::string(typeid(T).name()) + ":" + table_name;
        std::size_t hash_value = std::hash<std::string>{}(unique_key);

        // Create a type_index from a unique hash-based type identifier
        // We use the hash to create a unique identifier per table
        // Store in a map using hash as key
        auto hash_it = named_tables_.find(hash_value);
        if (hash_it != named_tables_.end()) {
            return *static_cast<Table<T>*>(hash_it->second.get());
        }

        // Create new table
        auto table_ptr = new Table<T>(storage_, table_name);
        Table<T>& table_ref = *table_ptr;

        // Store in maps using shared_ptr<void> with custom deleter
        named_tables_[hash_value] = std::shared_ptr<void>(table_ptr, [](void* p) {
            delete static_cast<Table<T>*>(p);
        });
        named_table_names_[hash_value] = table_name;

        // Set catalog pointer for count synchronization (skip system tables)
        if (catalog_ && table_name != "_sys_tables" && table_name != "_sys_fields") {
            table_ref.set_catalog(catalog_.get());
        }

        // Register in system catalog (skip system tables)
        if (catalog_ && table_name != "_sys_tables" && table_name != "_sys_fields") {
            catalog::TableMetadata meta{
                .table_name = table_name,
                .type_name = typeid(T).name(),
                .index_root_page = table_ref.get_root_page(),
                .record_count = 0,
                .created_timestamp = static_cast<uint64_t>(std::time(nullptr)),
                .is_system_table = false
            };

            // Extract field metadata if type supports reflection
            auto fields = reflection::extract_field_metadata<T>(table_name);

            // Register in catalog
            catalog_->register_table(meta, fields);
        }

        return table_ref;
    }

    /**
     * @brief Checks if a table exists for type T
     * @tparam T Type to check
     * @return true if table exists
     */
    template<typename T>
    [[nodiscard]] bool has_table() const {
        std::type_index type_id = std::type_index(typeid(T));
        return tables_.find(type_id) != tables_.end();
    }

    /**
     * @brief Flushes all tables to disk
     */
    void flush() {
        if (storage_) {
            storage_->flush_all();
        }
    }

    /**
     * @brief Gets the number of tables
     */
    [[nodiscard]] std::size_t table_count() const noexcept {
        return tables_.size() + named_tables_.size();
    }

    /**
     * @brief Gets the storage engine
     * @return Reference to the storage engine
     */
    [[nodiscard]] storage::StorageEngine& get_storage() noexcept {
        return *storage_;
    }

    /**
     * @brief Gets the storage engine (const)
     * @return Const reference to the storage engine
     */
    [[nodiscard]] const storage::StorageEngine& get_storage() const noexcept {
        return *storage_;
    }

    /**
     * @brief Gets the storage engine as a shared pointer
     * @return Shared pointer to the storage engine
     *
     * Use this when you need to create persistent secondary indexes
     * or other components that require shared ownership of the storage.
     *
     * Example:
     * @code
     * index::PersistentSecondaryIndex<Student, std::string> name_index(
     *     "name",
     *     [](const Student& s) { return s.get_name(); },
     *     db.get_storage_ptr()  // Shared pointer needed here
     * );
     * @endcode
     */
    [[nodiscard]] std::shared_ptr<storage::StorageEngine> get_storage_ptr() noexcept {
        return storage_;
    }

    /**
     * @brief Gets the database file path
     */
    [[nodiscard]] const std::string& get_file_path() const noexcept {
        return storage_->get_file_path();
    }

    /**
     * @brief Gets all table names
     * @return Vector of table names
     */
    [[nodiscard]] std::vector<std::string> get_table_names() const {
        std::vector<std::string> names;
        names.reserve(table_names_.size() + named_table_names_.size());

        for (const auto& [type_id, name] : table_names_) {
            names.push_back(name);
        }

        for (const auto& [hash, name] : named_table_names_) {
            names.push_back(name);
        }

        return names;
    }

    /**
     * @brief Drops a table from the database
     * @param table_name Name of the table to drop
     * @throws std::runtime_error if table is a system table or not found
     *
     * Example:
     * @code
     * db.drop_table("old_students");
     * @endcode
     */
    void drop_table(const std::string& table_name) {
        // Prevent dropping system tables
        if (table_name == "_sys_tables" || table_name == "_sys_fields") {
            throw std::runtime_error("Cannot drop system table: " + table_name);
        }

        // Find and remove the table from named_tables_
        bool found = false;
        std::size_t hash_to_remove = 0;

        for (const auto& [hash, name] : named_table_names_) {
            if (name == table_name) {
                hash_to_remove = hash;
                found = true;
                break;
            }
        }

        if (!found) {
            throw std::runtime_error("Table not found: " + table_name);
        }

        // Remove from catalog first
        if (catalog_) {
            catalog_->unregister_table(table_name);
        }

        // Remove from maps (this will destroy the Table object)
        named_tables_.erase(hash_to_remove);
        named_table_names_.erase(hash_to_remove);
    }

    /**
     * @brief Gets the system catalog for querying metadata
     * @return Reference to the system catalog
     *
     * Example:
     * @code
     * auto& catalog = db.metadata();
     * auto large_tables = catalog.tables()
     *     .where(catalog::TableMetadata::record_count > 1000);
     * @endcode
     */
    [[nodiscard]] catalog::SystemCatalog& metadata() {
        if (!catalog_) {
            throw std::runtime_error("System catalog not initialized");
        }
        return *catalog_;
    }

    /**
     * @brief Gets the system catalog (const version)
     */
    [[nodiscard]] const catalog::SystemCatalog& metadata() const {
        if (!catalog_) {
            throw std::runtime_error("System catalog not initialized");
        }
        return *catalog_;
    }

private:
    /**
     * @brief Generates a table name from type name
     * @tparam T Type
     * @return Table name
     */
    template<typename T>
    [[nodiscard]] static std::string get_table_name() {
        // Use type name as table name (simplified)
        // In production, you might want to demangle this
        std::string type_name = typeid(T).name();

        // Try to extract just the class name (platform-specific)
        // For now, just use the full type name
        return type_name;
    }

    /**
     * @brief Base class for type-erased table storage
     */
    struct TableBase {
        virtual ~TableBase() = default;
    };

    /**
     * @brief Type-erased table wrapper
     */
    template<typename T>
    struct TableWrapper : TableBase {
        Table<T> table;

        explicit TableWrapper(std::shared_ptr<storage::StorageEngine> storage, std::string name)
            : table(storage, std::move(name)) {}
    };

    /**
     * @brief Initialize system catalog (bootstrap or load)
     */
    void initialize_system_catalog() {
        uint64_t sys_tables_root = storage_->get_sys_tables_root();
        uint64_t sys_fields_root = storage_->get_sys_fields_root();
        uint64_t sys_indexes_root = storage_->get_sys_indexes_root();

        if (sys_tables_root == 0 || sys_fields_root == 0) {
            // New database - bootstrap system catalog
            bootstrap_system_catalog();
        } else {
            // Existing database - load system catalog
            // For v2 databases, sys_indexes_root will be 0 - create it lazily
            if (sys_indexes_root == 0) {
                // Upgrade from v2 to v3: bootstrap indexes table
                auto sys_indexes = Table<catalog::IndexMetadata>(storage_, "_sys_indexes");
                sys_indexes_root = sys_indexes.get_root_page();
                storage_->set_sys_indexes_root(sys_indexes_root);
            }

            catalog_ = std::make_unique<catalog::SystemCatalog>(
                storage_,
                sys_tables_root,
                sys_fields_root,
                sys_indexes_root
            );
        }
    }

    /**
     * @brief Bootstrap system catalog for new database
     */
    void bootstrap_system_catalog() {
        // Step 1: Create _sys_tables table to get root page
        {
            auto sys_tables = Table<catalog::TableMetadata>(storage_, "_sys_tables");
            uint64_t sys_tables_root = sys_tables.get_root_page();
            storage_->set_sys_tables_root(sys_tables_root);
        }

        // Step 2: Create _sys_fields table to get root page
        {
            auto sys_fields = Table<catalog::FieldMetadata>(storage_, "_sys_fields");
            uint64_t sys_fields_root = sys_fields.get_root_page();
            storage_->set_sys_fields_root(sys_fields_root);
        }

        // Step 3: Create _sys_indexes table to get root page (NEW!)
        {
            auto sys_indexes = Table<catalog::IndexMetadata>(storage_, "_sys_indexes");
            uint64_t sys_indexes_root = sys_indexes.get_root_page();
            storage_->set_sys_indexes_root(sys_indexes_root);
        }

        // Step 4: Create SystemCatalog with the stored root pages
        catalog_ = std::make_unique<catalog::SystemCatalog>(
            storage_,
            storage_->get_sys_tables_root(),
            storage_->get_sys_fields_root(),
            storage_->get_sys_indexes_root()
        );

        // Step 5: Register system tables in themselves
        register_system_table<catalog::TableMetadata>("_sys_tables");
        register_system_table<catalog::FieldMetadata>("_sys_fields");
        register_system_table<catalog::IndexMetadata>("_sys_indexes");  // NEW!
    }

    /**
     * @brief Register a system table in the catalog
     * @tparam T Table metadata type
     * @param table_name Name of the system table
     */
    template<typename T>
    void register_system_table(const std::string& table_name) {
        // Get the root page from storage
        uint64_t root_page = 0;
        if (table_name == "_sys_tables") {
            root_page = storage_->get_sys_tables_root();
        } else if (table_name == "_sys_fields") {
            root_page = storage_->get_sys_fields_root();
        }

        // Create metadata
        catalog::TableMetadata meta{
            .table_name = table_name,
            .type_name = typeid(T).name(),
            .index_root_page = root_page,
            .record_count = 0,
            .created_timestamp = static_cast<uint64_t>(std::time(nullptr)),
            .is_system_table = true
        };

        // Extract field metadata
        auto fields = reflection::extract_field_metadata<T>(table_name);

        // Register in catalog
        catalog_->register_table(meta, fields);
    }

private:
    std::shared_ptr<storage::StorageEngine> storage_;                         ///< Storage engine (shared with tables)
    std::unordered_map<std::type_index, std::shared_ptr<void>> tables_;      ///< Type-erased tables (unnamed)
    std::unordered_map<std::type_index, std::string> table_names_;           ///< Table names (unnamed)
    std::unordered_map<std::size_t, std::shared_ptr<void>> named_tables_;    ///< Named tables (hash-based)
    std::unordered_map<std::size_t, std::string> named_table_names_;         ///< Named table names
    std::unique_ptr<catalog::SystemCatalog> catalog_;                         ///< System catalog for metadata
};

} // namespace learnql::core

#endif // LEARNQL_CORE_DATABASE_HPP
