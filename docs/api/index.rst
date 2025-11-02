Index Module
============

.. contents:: Table of Contents
   :local:
   :depth: 2

Overview
--------

The Index module provides secondary indexes for fast data retrieval on non-primary-key fields. Indexes transform O(n) table scans into O(log n) lookups, dramatically improving query performance for large datasets. LearnQL's seamless index API integrates directly with the Table class using a fluent interface.

**Key Features:**

- Fluent API for index creation (``add_index()``)
- Automatic index persistence across restarts
- Unique and multi-value (non-unique) indexes
- Fast lookups: ``find_by()``, ``find_all_by()``
- Range queries with ``range_query()``
- Automatic CRUD synchronization
- System catalog integration

**Module Components:**

- Fluent index API on ``Table<T>`` class
- ``IndexType::Unique`` and ``IndexType::MultiValue``
- Query methods: ``find_by()``, ``find_all_by()``, ``range_query()``, ``get_unique_values()``
- Low-level persistent B-tree implementation (advanced users)

Quick Start
-----------

Creating Indexes with Fluent API
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <learnql/LearnQL.hpp>

   using namespace learnql;

   core::Database db("university.db");

   // Create table with indexes using fluent API
   auto& students = db.table<Student>("students")
       .add_index(Student::name, core::IndexType::Unique)
       .add_index(Student::department, core::IndexType::MultiValue)
       .add_index(Student::gpa, core::IndexType::MultiValue);

   // Indexes are ready to use immediately
   // They persist to disk and reload automatically

Using Indexes for Fast Lookups
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Fast unique lookup (O(log n))
   auto alice = students.find_by(Student::name, std::string("Alice Johnson"));
   if (alice) {
       std::cout << "Found: " << alice->get_name() << std::endl;
   }

   // Find all matching records (multi-value index)
   auto cs_students = students.find_all_by(
       Student::department,
       std::string("CS")
   );
   std::cout << "Found " << cs_students.size() << " CS students\n";

Range Queries
~~~~~~~~~~~~~

.. code-block:: cpp

   // Find students with GPA between 3.5 and 4.0
   auto high_achievers = students.range_query(Student::gpa, 3.5, 4.0);

   for (const auto& student : high_achievers) {
       std::cout << student.get_name() << ": " << student.get_gpa() << "\n";
   }

Getting Unique Values
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Get all unique departments
   auto departments = students.get_unique_values(Student::department);

   std::cout << "Departments:\n";
   for (const auto& dept : departments) {
       auto count = students.find_all_by(Student::department, dept).size();
       std::cout << "  - " << dept << " (" << count << " students)\n";
   }

Fluent Index API
----------------

add_index()
~~~~~~~~~~~

.. code-block:: cpp

   template<typename FieldType>
   Table<T>& add_index(
       const Field<T, FieldType>& field,
       core::IndexType index_type
   );

Creates a secondary index on a field.

**Parameters:**

- ``field``: Static Field object (e.g., ``Student::age``)
- ``index_type``: ``core::IndexType::Unique`` or ``core::IndexType::MultiValue``

**Returns:** Reference to ``this`` for method chaining

**Index Types:**

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Type
     - Use Case
   * - ``IndexType::Unique``
     - One value per key (e.g., email, student ID)
   * - ``IndexType::MultiValue``
     - Multiple values per key (e.g., department, city, age)

**Example:**

.. code-block:: cpp

   auto& students = db.table<Student>("students")
       .add_index(Student::email, core::IndexType::Unique)       // Unique constraint
       .add_index(Student::department, core::IndexType::MultiValue)  // Many students per dept
       .add_index(Student::age, core::IndexType::MultiValue);    // Many students per age

**Features:**

- Returns ``Table&`` for fluent chaining
- Index persists to disk automatically
- Index metadata registered in system catalog
- Existing data indexed immediately
- On table reopen, indexes automatically reconstructed

**Persistence:**

.. code-block:: cpp

   // First run - create table with indexes
   auto& students = db.table<Student>("students")
       .add_index(Student::name, core::IndexType::Unique);

   // Insert data
   students.insert({1001, "Alice", "CS", 20, 3.8});
   db.flush();  // Indexes saved to disk

   // Second run - reopen database
   core::Database db2("university.db");
   auto& students2 = db2.table<Student>("students")
       .add_index(Student::name, core::IndexType::Unique);  // Reloads existing index

   // Index still works!
   auto alice = students2.find_by(Student::name, std::string("Alice"));

find_by()
~~~~~~~~~

.. code-block:: cpp

   template<typename FieldType, typename ValueType>
   std::optional<T> find_by(
       const Field<T, FieldType>& field,
       const ValueType& value
   );

Finds a single record using a unique index.

**Parameters:**

- ``field``: Static Field object with unique index
- ``value``: Value to search for

**Returns:** ``std::optional<T>`` containing record if found, ``std::nullopt`` otherwise

**Requires:** Unique index on ``field`` (created with ``IndexType::Unique``)

**Time Complexity:** O(log n)

**Example:**

.. code-block:: cpp

   // Requires: .add_index(Student::name, core::IndexType::Unique)
   auto student = students.find_by(Student::name, std::string("Alice Johnson"));

   if (student) {
       std::cout << "Found: " << student->get_name() << "\n";
       std::cout << "GPA: " << student->get_gpa() << "\n";
   } else {
       std::cout << "Student not found\n";
   }

**Performance:**

.. code-block:: text

   Without index: O(n) - full table scan
   With index:    O(log n) - B-tree lookup

   For 1M records:
   - No index: ~1,000,000 comparisons
   - With index: ~20 comparisons (50,000x faster!)

find_all_by()
~~~~~~~~~~~~~

.. code-block:: cpp

   template<typename FieldType, typename ValueType>
   std::vector<T> find_all_by(
       const Field<T, FieldType>& field,
       const ValueType& value
   );

Finds all records matching a value using a multi-value index.

**Parameters:**

- ``field``: Static Field object with multi-value index
- ``value``: Value to search for

**Returns:** Vector of all matching records

**Requires:** Multi-value index on ``field`` (created with ``IndexType::MultiValue``)

**Time Complexity:** O(log n + k) where k = number of matches

**Example:**

.. code-block:: cpp

   // Requires: .add_index(Student::department, core::IndexType::MultiValue)
   auto cs_students = students.find_all_by(
       Student::department,
       std::string("CS")
   );

   std::cout << "CS Students (" << cs_students.size() << " total):\n";
   for (const auto& s : cs_students) {
       std::cout << "  - " << s.get_name()
                 << " (GPA: " << s.get_gpa() << ")\n";
   }

**Use Cases:**

- Finding all students in a department
- Finding all orders for a customer
- Finding all products in a category
- Finding all users in a city

range_query()
~~~~~~~~~~~~~

.. code-block:: cpp

   template<typename FieldType, typename ValueType>
   std::vector<T> range_query(
       const Field<T, FieldType>& field,
       const ValueType& min_value,
       const ValueType& max_value
   );

Finds all records with field values in range [min, max] (inclusive).

**Parameters:**

- ``field``: Static Field object with index
- ``min_value``: Minimum value (inclusive)
- ``max_value``: Maximum value (inclusive)

**Returns:** Vector of matching records in sorted order

**Requires:** Index on ``field`` (unique or multi-value)

**Time Complexity:** O(log n + k) where k = number of results

**Example:**

.. code-block:: cpp

   // Requires: .add_index(Student::gpa, core::IndexType::MultiValue)

   // Find students with GPA between 3.5 and 4.0
   auto high_achievers = students.range_query(Student::gpa, 3.5, 4.0);

   std::cout << "High Achievers (GPA 3.5-4.0):\n";
   for (const auto& s : high_achievers) {
       std::cout << "  " << s.get_name()
                 << ": " << s.get_gpa() << "\n";
   }

   // Age range query
   auto young_adults = students.range_query(Student::age, 18, 25);

**Use Cases:**

- Finding products in a price range
- Finding events in a date range
- Finding employees by salary range
- Finding records by score/rating range

get_unique_values()
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   template<typename FieldType>
   std::vector<FieldType> get_unique_values(
       const Field<T, FieldType>& field
   );

Gets all distinct values for an indexed field.

**Parameters:**

- ``field``: Static Field object with index

**Returns:** Vector of unique values

**Requires:** Index on ``field``

**Time Complexity:** O(n) where n = number of unique values

**Example:**

.. code-block:: cpp

   // Requires: .add_index(Student::department, core::IndexType::MultiValue)

   auto departments = students.get_unique_values(Student::department);

   std::cout << "Departments:\n";
   for (const auto& dept : departments) {
       auto count = students.find_all_by(Student::department, dept).size();
       std::cout << "  - " << std::setw(15) << std::left << dept
                 << " (" << count << " students)\n";
   }

   // Output:
   // Departments:
   //   - CS             (45 students)
   //   - Math           (32 students)
   //   - Physics        (28 students)

**Use Cases:**

- Listing all categories
- Finding all unique tags
- Discovering distinct values for faceted search
- Generating filter options for UI

Automatic CRUD Synchronization
-------------------------------

Indexes automatically update when records change:

insert()
~~~~~~~~

.. code-block:: cpp

   students.insert({1001, "Alice Johnson", "CS", 20, 3.8});
   // All indexes automatically updated:
   // - name index: "Alice Johnson" -> RecordId{...}
   // - department index: "CS" -> RecordId{...}
   // - gpa index: 3.8 -> RecordId{...}

update()
~~~~~~~~

.. code-block:: cpp

   auto student = students.find(1001);
   if (student) {
       student->set_department("Engineering");  // Change department
       students.update(*student);
       // Indexes automatically updated:
       // - Removed from "CS" in department index
       // - Added to "Engineering" in department index
   }

remove()
~~~~~~~~

.. code-block:: cpp

   students.remove(1001);
   // All indexes automatically cleaned up:
   // - name index entry removed
   // - department index entry removed
   // - gpa index entry removed

**Performance Impact:**

.. list-table::
   :header-rows: 1
   :widths: 40 30 30

   * - Operation
     - Without Indexes
     - With 3 Indexes
   * - Insert
     - O(1)
     - O(3 × log n)
   * - Update
     - O(n)
     - O(n + 3 × log n)
   * - Delete
     - O(n)
     - O(n + 3 × log n)
   * - Find by indexed field
     - O(n)
     - O(log n)

**Guideline:** Create indexes on frequently queried fields, but avoid over-indexing.

Complete Example
----------------

Full Workflow
~~~~~~~~~~~~~

.. code-block:: cpp

   #include <learnql/LearnQL.hpp>
   #include <iostream>
   #include <iomanip>

   using namespace learnql;

   class Student {
       LEARNQL_PROPERTIES_BEGIN(Student)
           LEARNQL_PROPERTY(int, student_id, PK)
           LEARNQL_PROPERTY(std::string, name)
           LEARNQL_PROPERTY(std::string, department)
           LEARNQL_PROPERTY(int, age)
           LEARNQL_PROPERTY(double, gpa)
       LEARNQL_PROPERTIES_END(
           PROP(int, student_id, PK),
           PROP(std::string, name),
           PROP(std::string, department),
           PROP(int, age),
           PROP(double, gpa)
       )
   };

   int main() {
       // 1. Create database and table with indexes
       core::Database db("university.db");

       auto& students = db.table<Student>("students")
           .add_index(Student::name, core::IndexType::Unique)
           .add_index(Student::department, core::IndexType::MultiValue)
           .add_index(Student::gpa, core::IndexType::MultiValue);

       // 2. Insert data (indexes auto-update)
       students.insert({1001, "Alice Johnson", "CS", 20, 3.8});
       students.insert({1002, "Bob Smith", "CS", 21, 3.5});
       students.insert({1003, "Carol White", "Math", 19, 3.9});
       students.insert({1004, "David Brown", "Physics", 22, 3.2});

       // 3. Fast unique lookup
       std::cout << "=== Unique Lookup ===\n";
       auto alice = students.find_by(Student::name, std::string("Alice Johnson"));
       if (alice) {
           std::cout << "Found: " << alice->get_name()
                     << " (GPA: " << alice->get_gpa() << ")\n\n";
       }

       // 4. Multi-value lookup
       std::cout << "=== Multi-Value Lookup ===\n";
       auto cs_students = students.find_all_by(
           Student::department,
           std::string("CS")
       );
       std::cout << "CS Students:\n";
       for (const auto& s : cs_students) {
           std::cout << "  - " << s.get_name() << "\n";
       }

       // 5. Range query
       std::cout << "\n=== Range Query ===\n";
       auto high_gpa = students.range_query(Student::gpa, 3.5, 4.0);
       std::cout << "Students with GPA 3.5-4.0:\n";
       for (const auto& s : high_gpa) {
           std::cout << "  - " << s.get_name()
                     << ": " << std::fixed << std::setprecision(2)
                     << s.get_gpa() << "\n";
       }

       // 6. Get unique values
       std::cout << "\n=== Unique Values ===\n";
       auto departments = students.get_unique_values(Student::department);
       std::cout << "Departments:\n";
       for (const auto& dept : departments) {
           auto count = students.find_all_by(Student::department, dept).size();
           std::cout << "  - " << dept << " (" << count << " students)\n";
       }

       // 7. Update with automatic index sync
       std::cout << "\n=== Update with Index Sync ===\n";
       if (alice) {
           std::cout << "Before: " << alice->get_department() << "\n";
           alice->set_department("Engineering");
           students.update(*alice);

           auto updated = students.find_by(
               Student::name,
               std::string("Alice Johnson")
           );
           if (updated) {
               std::cout << "After: " << updated->get_department() << "\n";
           }
       }

       // 8. Verify persistence
       db.flush();
       std::cout << "\n✓ All indexes persisted to disk\n";

       return 0;
   }

Index Metadata in System Catalog
---------------------------------

Query Index Metadata
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   auto& catalog = db.metadata();

   // Query indexes for a table
   using namespace catalog;
   auto student_indexes = catalog.indexes()
       .where(IndexMetadata::table == "students");

   std::cout << "Indexes for 'students' table:\n";
   for (const auto& idx : student_indexes) {
       std::cout << "  • Field: " << idx.get_field_name()
                 << " | Type: " << (idx.get_is_unique() ? "Unique" : "MultiValue")
                 << " | Root Page: " << idx.get_index_root_page() << "\n";
   }

List All Indexes
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   auto all_indexes = catalog.indexes().get_all();

   std::cout << "All Indexes:\n";
   for (const auto& idx : all_indexes) {
       std::cout << "  " << idx.get_table_name() << "."
                 << idx.get_field_name()
                 << " (" << (idx.get_is_unique() ? "unique" : "multi-value") << ")\n";
   }

When to Use Indexes
-------------------

Create Indexes When
~~~~~~~~~~~~~~~~~~~

✅ **Frequent searches on a field**

.. code-block:: cpp

   // If you frequently search by department
   students.add_index(Student::department, core::IndexType::MultiValue);

✅ **Large tables (1000+ rows)**

.. code-block:: text

   No index:   O(n) = 100,000 comparisons
   With index: O(log n) ≈ 17 comparisons (5,900x faster!)

✅ **Range queries**

.. code-block:: cpp

   // Range queries benefit from indexes
   auto results = students.range_query(Student::gpa, 3.0, 4.0);

✅ **Unique constraints**

.. code-block:: cpp

   // Enforce uniqueness on email
   students.add_index(Student::email, core::IndexType::Unique);

Avoid Indexes When
~~~~~~~~~~~~~~~~~~

❌ **Small tables (< 100 rows)**

Linear scan is faster than index overhead.

❌ **Field values are mostly unique and you query for many records**

If every query returns 50%+ of rows, index doesn't help.

❌ **High write frequency, low read frequency**

Index maintenance cost outweighs read benefits.

❌ **Limited disk space**

Indexes consume additional storage (~20-30% of table size).

Performance Characteristics
----------------------------

Time Complexity
~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 40 30 30

   * - Operation
     - Without Index
     - With Index
   * - find_by (unique)
     - O(n)
     - O(log n)
   * - find_all_by
     - O(n)
     - O(log n + k)
   * - range_query
     - O(n)
     - O(log n + k)
   * - get_unique_values
     - O(n)
     - O(u)
   * - Insert
     - O(1)
     - O(log n)
   * - Update
     - O(n)
     - O(n + log n)
   * - Delete
     - O(n)
     - O(n + log n)

k = number of matching results, u = number of unique values

Space Complexity
~~~~~~~~~~~~~~~~

**Per index entry:** ~16-32 bytes (key + RecordId + B-tree overhead)

**Total index size:**

.. code-block:: text

   Index Size ≈ (n × entry_size) + (n / branching_factor × 4KB)

   Example for 100,000 entries:
   - Entry size: 20 bytes
   - Branching factor: 128
   - Total: ~2MB for entries + ~3MB for nodes ≈ 5MB

Real-World Performance
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: text

   Table size: 1,000,000 records

   Without index (find_by equivalent):
   - Average comparisons: 500,000
   - Time: ~500ms

   With unique index (find_by):
   - Average comparisons: 20
   - Time: ~1ms
   - Speedup: 500x

Advanced: Low-Level Index API
------------------------------

For advanced users who need direct B-tree access:

PersistentSecondaryIndex
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <learnql/index/PersistentSecondaryIndex.hpp>

   // Low-level unique index
   index::PersistentSecondaryIndex<Student, std::string> name_index(
       "name",
       [](const Student& s) { return s.get_name(); },
       db.get_storage_ptr()
   );

   // Manual index population
   for (const auto& student : students) {
       auto rid = students.get_record_id(student.get_student_id());
       if (rid) {
           name_index.insert(student, *rid);
       }
   }

**Note:** The fluent API (``add_index()``) is recommended for most users. Use low-level API only for custom index implementations.

Best Practices
--------------

1. **Create Indexes on Frequently Queried Fields**

   .. code-block:: cpp

      // Analyze your queries first
      auto cs_students = students.find_all_by(Student::department, "CS");  // Frequent?

      // Then create indexes
      students.add_index(Student::department, core::IndexType::MultiValue);

2. **Use Fluent API for Chaining**

   .. code-block:: cpp

      auto& table = db.table<Student>("students")
          .add_index(Student::name, core::IndexType::Unique)
          .add_index(Student::department, core::IndexType::MultiValue)
          .add_index(Student::age, core::IndexType::MultiValue);

3. **Choose Correct Index Type**

   .. code-block:: cpp

      // Unique - one value per key
      .add_index(Student::email, core::IndexType::Unique)

      // MultiValue - many values per key
      .add_index(Student::department, core::IndexType::MultiValue)

4. **Let Indexes Handle CRUD**

   .. code-block:: cpp

      // Just use normal CRUD operations
      students.insert(student);    // Indexes auto-update
      students.update(student);    // Indexes auto-update
      students.remove(id);         // Indexes auto-cleanup

5. **Verify Persistence**

   .. code-block:: cpp

      db.flush();  // Ensure indexes saved to disk

Troubleshooting
---------------

Index Not Being Used
~~~~~~~~~~~~~~~~~~~~

**Symptoms:** Query is slow despite having an index

**Solution:** Check that index exists in catalog

.. code-block:: cpp

   auto indexes = db.metadata().indexes()
       .where(IndexMetadata::table == "students");

   for (const auto& idx : indexes) {
       std::cout << "Index on: " << idx.get_field_name() << "\n";
   }

No Index on Field
~~~~~~~~~~~~~~~~~

**Problem:** ``find_by()`` throws error

**Solution:** Create index on the field

.. code-block:: cpp

   students.add_index(Student::name, core::IndexType::Unique);

Wrong Index Type
~~~~~~~~~~~~~~~~

**Problem:** ``find_by()`` returns multiple results

**Cause:** Field has ``MultiValue`` index instead of ``Unique``

**Solution:** Use ``find_all_by()`` or recreate with ``Unique`` type

See Also
--------

- :doc:`core` - Table class with index methods
- :doc:`query` - Query operations that use indexes
- :doc:`catalog` - System catalog (index metadata)
- :doc:`storage` - Storage engine (internal)
- :doc:`../tutorials/indexing` - Indexing tutorial
- :doc:`../guides/performance` - Performance optimization guide

**Related Classes:**

- ``Table<T>`` - High-level table API with indexing
- ``Database`` - Database management
- ``SystemCatalog`` - Index metadata storage
