Catalog Module
==============

.. contents:: Table of Contents
   :local:
   :depth: 2

Overview
--------

The Catalog module provides a queryable system catalog that stores metadata about tables, fields, and indexes. Just like in professional database systems (PostgreSQL's ``pg_catalog``, MySQL's ``information_schema``), LearnQL's catalog tables are themselves queryable using the same expression template interface as regular tables.

**Key Features:**

- Queryable metadata tables using standard LearnQL query syntax
- Three system tables: ``_sys_tables``, ``_sys_fields``, ``_sys_indexes``
- Automatic maintenance (updated on table/index operations)
- Read-only access to prevent corruption
- Full integration with C++20 ranges

**System Tables:**

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Table
     - Description
   * - ``_sys_tables``
     - Metadata about all user tables
   * - ``_sys_fields``
     - Metadata about all fields/columns
   * - ``_sys_indexes``
     - Metadata about secondary indexes

Quick Start
-----------

Accessing the Catalog
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <learnql/LearnQL.hpp>

   Database db("school.db");
   auto& catalog = db.metadata();

   // Query tables
   auto large_tables = catalog.tables()
       .where(TableMetadata::record_count > 1000);

   for (const auto& table : large_tables) {
       std::cout << table.table_name << ": "
                 << table.record_count << " rows\n";
   }

Querying Field Metadata
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Get all fields for a specific table
   auto student_fields = catalog.fields()
       .where(FieldMetadata::table == "students");

   for (const auto& field : student_fields) {
       std::cout << field.field_name << " ("
                 << field.field_type << ")"
                 << (field.is_primary_key ? " PK" : "")
                 << "\n";
   }

Querying Index Metadata
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // List all indexes
   auto all_indexes = catalog.indexes().all();

   for (const auto& idx : all_indexes) {
       std::cout << idx.get_table_name() << "."
                 << idx.get_field_name()
                 << " (" << (idx.is_unique ? "unique" : "multi-value") << ")\n";
   }

Class Reference
---------------

SystemCatalog Class
~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: learnql::catalog::SystemCatalog
   :members:

The ``SystemCatalog`` class provides read-only access to metadata about the database schema.

**Constructor:**

.. code-block:: cpp

   SystemCatalog(
       std::shared_ptr<storage::StorageEngine> storage,
       uint64_t sys_tables_root,
       uint64_t sys_fields_root,
       uint64_t sys_indexes_root
   );

**Note:** Typically created by ``Database`` class - users don't construct directly.

Public Methods
^^^^^^^^^^^^^^

``tables()`` - Access Table Metadata
"""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   const core::ReadOnlyTable<TableMetadata>& tables() const;

Returns a read-only table containing metadata about all user tables.

**Returns:** Read-only table of ``TableMetadata`` records

**Example:**

.. code-block:: cpp

   auto& tables = catalog.tables();

   // Query like any other table
   auto recent = tables.where(TableMetadata::created_at > timestamp);

``fields()`` - Access Field Metadata
"""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   const core::ReadOnlyTable<FieldMetadata>& fields() const;

Returns a read-only table containing metadata about all fields in all tables.

**Returns:** Read-only table of ``FieldMetadata`` records

**Example:**

.. code-block:: cpp

   auto& fields = catalog.fields();

   // Find all primary keys
   auto pks = fields.where(FieldMetadata::is_primary_key == true);

``indexes()`` - Access Index Metadata
""""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   const core::ReadOnlyTable<IndexMetadata>& indexes() const;

Returns a read-only table containing metadata about all secondary indexes.

**Returns:** Read-only table of ``IndexMetadata`` records

**Example:**

.. code-block:: cpp

   auto& indexes = catalog.indexes();

   // Find all unique indexes
   auto unique_indexes = indexes.where(IndexMetadata::is_unique == true);

Metadata Structures
-------------------

TableMetadata
~~~~~~~~~~~~~

.. doxygenstruct:: learnql::catalog::TableMetadata
   :members:

Stores metadata about a table.

**Fields:**

.. code-block:: cpp

   struct TableMetadata {
       std::string table_name;      // Table name (primary key)
       std::string table_type;      // C++ type name
       uint64_t root_page_id;       // Root page ID for data
       uint64_t record_count;       // Number of records
       uint64_t created_at;         // Creation timestamp
       bool is_system_table;        // true for _sys_* tables

       // Property descriptors (for queries)
       static inline Property<TableMetadata, std::string> table_name;
       static inline Property<TableMetadata, uint64_t> record_count;
       // ... (see full struct for all properties)
   };

**Example:**

.. code-block:: cpp

   TableMetadata meta;
   meta.table_name = "students";
   meta.table_type = "Student";
   meta.record_count = 1500;
   meta.is_system_table = false;

FieldMetadata
~~~~~~~~~~~~~

.. doxygenstruct:: learnql::catalog::FieldMetadata
   :members:

Stores metadata about a field/column.

**Fields:**

.. code-block:: cpp

   struct FieldMetadata {
       uint64_t field_id;           // Unique field ID (primary key)
       std::string table_name;      // Owning table
       std::string field_name;      // Field name
       std::string field_type;      // C++ type name (e.g., "int", "std::string")
       uint32_t field_order;        // Position in struct (0-based)
       bool is_primary_key;         // true if this is the PK

       // Property descriptors
       static inline Property<FieldMetadata, std::string> table;
       static inline Property<FieldMetadata, std::string> field_name;
       static inline Property<FieldMetadata, bool> is_primary_key;
       // ...
   };

**Example:**

.. code-block:: cpp

   FieldMetadata field;
   field.field_id = 42;
   field.table_name = "students";
   field.field_name = "age";
   field.field_type = "int";
   field.field_order = 2;
   field.is_primary_key = false;

IndexMetadata
~~~~~~~~~~~~~

.. doxygenstruct:: learnql::catalog::IndexMetadata
   :members:

Stores metadata about a secondary index.

**Fields:**

.. code-block:: cpp

   struct IndexMetadata {
       uint64_t index_id;           // Unique index ID (primary key)
       std::string table_name;      // Table being indexed
       std::string field_name;      // Indexed field
       std::string field_type;      // Field type (for deserialization)
       bool is_unique;              // true = unique, false = multi-value
       uint64_t root_page_id;       // Root page of B-tree
       uint64_t created_at;         // Creation timestamp
       bool is_active;              // Index status

       // Getters (for const access)
       uint64_t get_index_id() const;
       const std::string& get_table_name() const;
       const std::string& get_field_name() const;
       bool get_is_unique() const;
       // ...

       // Property descriptors
       static inline Property<IndexMetadata, std::string> table;
       static inline Property<IndexMetadata, std::string> field;
       static inline Property<IndexMetadata, bool> is_unique;
       // ...
   };

**Example:**

.. code-block:: cpp

   IndexMetadata idx;
   idx.index_id = 1;
   idx.table_name = "students";
   idx.field_name = "age";
   idx.field_type = "int";
   idx.is_unique = false;  // Multi-value index
   idx.is_active = true;

Usage Examples
--------------

Listing All Tables
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   auto& catalog = db.metadata();

   std::cout << "=== Database Tables ===\n";
   for (const auto& table : catalog.tables().all()) {
       std::cout << table.table_name
                 << " (" << table.record_count << " rows)\n";
   }

Inspecting Table Schema
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void print_schema(const std::string& table_name, SystemCatalog& catalog) {
       std::cout << "Table: " << table_name << "\n";
       std::cout << "Columns:\n";

       auto fields = catalog.fields()
           .where(FieldMetadata::table == table_name);

       for (const auto& field : fields) {
           std::cout << "  " << field.field_name
                     << " " << field.field_type;

           if (field.is_primary_key) {
               std::cout << " PRIMARY KEY";
           }

           std::cout << "\n";
       }

       // List indexes
       auto indexes = catalog.indexes()
           .where(IndexMetadata::table == table_name);

       if (!indexes.empty()) {
           std::cout << "Indexes:\n";
           for (const auto& idx : indexes) {
               std::cout << "  " << idx.get_field_name()
                         << " (" << (idx.get_is_unique() ? "unique" : "multi-value")
                         << ")\n";
           }
       }
   }

   // Usage
   print_schema("students", catalog);

Finding Tables with Indexes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Get all indexed tables
   auto indexed_tables = catalog.indexes()
       | std::views::transform([](auto& idx) { return idx.get_table_name(); })
       | std::ranges::to<std::set>();  // Remove duplicates

   std::cout << "Tables with indexes:\n";
   for (const auto& table_name : indexed_tables) {
       std::cout << "  " << table_name << "\n";
   }

Statistics Gathering
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <numeric>

   auto& catalog = db.metadata();

   // Total database size (record count)
   auto all_tables = catalog.tables().all();
   std::size_t total_records = std::accumulate(
       all_tables.begin(), all_tables.end(), 0ULL,
       [](auto sum, auto& t) { return sum + t.record_count; }
   );

   std::cout << "Total records: " << total_records << "\n";

   // Average records per table
   double avg_records = static_cast<double>(total_records) / all_tables.size();
   std::cout << "Avg records/table: " << avg_records << "\n";

   // Total indexes
   std::size_t index_count = catalog.indexes().size();
   std::cout << "Total indexes: " << index_count << "\n";

Finding Large Tables
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   auto large_tables = catalog.tables()
       .where(TableMetadata::record_count > 10000);

   std::cout << "Large tables (>10,000 rows):\n";
   for (const auto& table : large_tables) {
       std::cout << "  " << table.table_name
                 << ": " << table.record_count << " rows\n";
   }

Finding Primary Keys
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Find primary key for each table
   std::map<std::string, std::string> primary_keys;

   auto pk_fields = catalog.fields()
       .where(FieldMetadata::is_primary_key == true);

   for (const auto& field : pk_fields) {
       primary_keys[field.table_name] = field.field_name;
   }

   std::cout << "Primary Keys:\n";
   for (const auto& [table, pk] : primary_keys) {
       std::cout << "  " << table << ": " << pk << "\n";
   }

Integration with Ranges
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <ranges>

   auto& catalog = db.metadata();

   // Find top 5 largest tables
   auto top_tables = catalog.tables().view()
       | std::views::transform([](auto& t) {
           return std::pair{t.table_name, t.record_count};
         })
       | std::ranges::to<std::vector>()
       | std::views::reverse;  // Largest first

   std::sort(top_tables.begin(), top_tables.end(),
             [](auto& a, auto& b) { return a.second > b.second; });

   std::cout << "Top 5 largest tables:\n";
   for (const auto& [name, count] : top_tables | std::views::take(5)) {
       std::cout << "  " << name << ": " << count << " rows\n";
   }

Advanced Queries
----------------

Complex Filtering
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Find all non-system tables with > 100 rows
   auto active_tables = catalog.tables()
       .where(
           (TableMetadata::is_system_table == false) &&
           (TableMetadata::record_count > 100)
       );

   // Find all string fields
   auto string_fields = catalog.fields()
       .where(FieldMetadata::field_type == "std::string");

   // Find all unique indexes on integer fields
   auto int_unique_indexes = catalog.indexes()
       .where(
           (IndexMetadata::is_unique == true) &&
           (IndexMetadata::field_type == "int")
       );

Cross-Table Queries
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Manually join tables and fields
   auto tables = catalog.tables().all();
   auto fields = catalog.fields().all();

   for (const auto& table : tables) {
       std::cout << "Table: " << table.table_name << "\n";

       // Find fields for this table
       auto table_fields = fields
           | std::views::filter([&](auto& f) {
               return f.table_name == table.table_name;
             });

       for (const auto& field : table_fields) {
           std::cout << "  - " << field.field_name << "\n";
       }
   }

Validation Queries
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Check for orphaned fields (fields referencing non-existent tables)
   auto all_table_names = catalog.tables()
       | std::views::transform([](auto& t) { return t.table_name; })
       | std::ranges::to<std::set>();

   auto all_fields = catalog.fields().all();
   for (const auto& field : all_fields) {
       if (!all_table_names.contains(field.table_name)) {
           std::cerr << "WARNING: Orphaned field: "
                     << field.table_name << "." << field.field_name << "\n";
       }
   }

Database Inspector
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <learnql/utils/DbInspector.hpp>

   Database db("app.db");

   // Use built-in inspector for comprehensive output
   utils::DbInspector inspector(db);
   inspector.print_summary();
   inspector.print_tables();
   inspector.print_indexes();

   // Output:
   // === Database Summary ===
   // Tables: 5
   // Total Records: 12,543
   // Indexes: 7
   // ...

System Catalog Implementation
------------------------------

Storage
~~~~~~~

The system catalog uses three special tables stored in the database file:

.. code-block:: text

   Database File
   ┌────────────────────────────────┐
   │ Page 0: Metadata               │
   │   - sys_tables_root = 10       │ ─┐
   │   - sys_fields_root = 20       │  │
   │   - sys_indexes_root = 30      │  │
   ├────────────────────────────────┤  │
   │ ...                            │  │
   ├────────────────────────────────┤  │
   │ Page 10: _sys_tables (root)    │ ←┘
   │   - students: 1000 rows        │
   │   - courses: 500 rows          │
   │   - ...                        │
   ├────────────────────────────────┤
   │ Page 20: _sys_fields (root)    │ ←─ Field metadata
   │   - students.id (int, PK)      │
   │   - students.name (string)     │
   │   - ...                        │
   ├────────────────────────────────┤
   │ Page 30: _sys_indexes (root)   │ ←─ Index metadata
   │   - students.age (multi-value) │
   │   - students.id (unique)       │
   └────────────────────────────────┘

Bootstrapping
~~~~~~~~~~~~~

When creating a new database, the system catalog is initialized first:

.. code-block:: cpp

   // Pseudo-code for database creation
   1. Create storage engine
   2. Create empty _sys_tables table
   3. Create empty _sys_fields table
   4. Create empty _sys_indexes table
   5. Register system tables in catalog
   6. Save root page IDs to metadata page

Automatic Maintenance
~~~~~~~~~~~~~~~~~~~~~

The catalog is automatically updated when:

.. code-block:: cpp

   // Creating a table
   auto& students = db.table<Student>("students");
   // → Inserts TableMetadata and FieldMetadata records

   // Creating an index
   students.create_index(&Student::get_age);
   // → Inserts IndexMetadata record

   // Dropping an index
   students.drop_index(&Student::get_age);
   // → Removes IndexMetadata record

   // Inserting/deleting records
   students.insert(student);
   // → Updates TableMetadata::record_count

Read-Only Protection
~~~~~~~~~~~~~~~~~~~~

System catalog tables are wrapped in ``ReadOnlyTable`` to prevent accidental modification:

.. code-block:: cpp

   auto& tables = catalog.tables();

   // OK - read operations
   auto all = tables.all();
   auto filtered = tables.where(TableMetadata::record_count > 100);

   // COMPILER ERROR - write operations not allowed
   // tables.insert(meta);  // Won't compile!
   // tables.update(meta);  // Won't compile!
   // tables.remove("students");  // Won't compile!

Only the ``Database`` class (via ``friend`` access) can modify the catalog.

Performance Considerations
--------------------------

Catalog Query Performance
~~~~~~~~~~~~~~~~~~~~~~~~~

System catalog tables are small (typically < 100 records), so full table scans are fast:

.. list-table::
   :header-rows: 1
   :widths: 40 30 30

   * - Database Size
     - Catalog Size
     - Query Time
   * - 10 tables
     - ~50 records
     - < 1ms
   * - 100 tables
     - ~500 records
     - ~1-2ms
   * - 1000 tables
     - ~5000 records
     - ~10-20ms

Catalog queries don't benefit significantly from indexes due to small size.

Caching
~~~~~~~

Catalog data can be cached in application code:

.. code-block:: cpp

   // Cache table names (avoid repeated catalog queries)
   static std::vector<std::string> cached_table_names;

   if (cached_table_names.empty()) {
       cached_table_names = catalog.tables()
           | std::views::transform([](auto& t) { return t.table_name; })
           | std::ranges::to<std::vector>();
   }

Best Practices
--------------

1. **Query Catalog on Startup**

   .. code-block:: cpp

      // Validate schema on startup
      void validate_database(Database& db) {
          auto& catalog = db.metadata();

          // Check required tables exist
          auto tables = catalog.tables()
              | std::views::transform([](auto& t) { return t.table_name; })
              | std::ranges::to<std::set>();

          if (!tables.contains("users")) {
              throw std::runtime_error("Missing required table: users");
          }
       }

2. **Use Catalog for Schema Discovery**

   .. code-block:: cpp

      // Dynamically discover available tables
      void list_available_operations(Database& db) {
          auto tables = db.metadata().tables().all();

          std::cout << "Available operations:\n";
          for (const auto& table : tables) {
              if (!table.is_system_table) {
                  std::cout << "  - " << table.table_name << "\n";
              }
          }
      }

3. **Document Schema Changes via Catalog**

   .. code-block:: cpp

      // Generate schema documentation
      void generate_schema_doc(Database& db, std::ostream& out) {
          auto& catalog = db.metadata();

          out << "# Database Schema\n\n";

          for (const auto& table : catalog.tables().all()) {
              if (table.is_system_table) continue;

              out << "## " << table.table_name << "\n\n";
              out << "- Records: " << table.record_count << "\n\n";
              out << "### Fields\n\n";

              auto fields = catalog.fields()
                  .where(FieldMetadata::table == table.table_name);

              for (const auto& field : fields) {
                  out << "- `" << field.field_name
                      << "` (" << field.field_type << ")";
                  if (field.is_primary_key) out << " **PK**";
                  out << "\n";
              }

              out << "\n";
          }
      }

4. **Don't Attempt to Modify Directly**

   .. code-block:: cpp

      // WRONG - catalog is read-only
      // catalog.tables().insert(meta);  // Won't compile

      // RIGHT - use Database API
      db.table<MyType>("my_table");  // Automatically updates catalog

Troubleshooting
---------------

Catalog Out of Sync
~~~~~~~~~~~~~~~~~~~

**Symptoms:** Catalog shows incorrect record counts or missing tables

**Causes:**

- Application crash during table modification
- Manual database file editing
- Bug in catalog maintenance code

**Solutions:**

.. code-block:: cpp

   // Option 1: Recreate database (if data can be lost)
   std::filesystem::remove("app.db");
   Database db("app.db");  // Fresh start

   // Option 2: Use DbInspector to diagnose
   utils::DbInspector inspector(db);
   inspector.print_summary();

   // Option 3: Manual repair (advanced)
   // Drop and recreate affected tables

Missing System Tables
~~~~~~~~~~~~~~~~~~~~~

**Symptoms:** ``std::runtime_error`` when accessing catalog

**Cause:** Database file corrupted or incompatible version

**Solution:** Recreate database with current version

Slow Catalog Queries
~~~~~~~~~~~~~~~~~~~~

**Symptoms:** Catalog queries take > 100ms

**Cause:** Extremely large number of tables (1000+)

**Solutions:**

- Cache catalog results in memory
- Reduce number of tables (consolidate if possible)
- Use external metadata storage for very large schemas

See Also
--------

- :doc:`reflection` - Compile-time reflection for generating field metadata
- :doc:`query` - Query interface used to query catalog
- :doc:`storage` - Storage engine (page-based storage)
- :doc:`../tutorials/schema` - Schema design tutorial
- :doc:`../guides/architecture` - System architecture

**Related Classes:**

- ``Database`` - Main database class that provides catalog access
- ``ReadOnlyTable<T>`` - Read-only table wrapper
- ``DbInspector`` - Database inspection utility
