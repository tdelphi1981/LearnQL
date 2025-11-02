Tutorial 6: Secondary Indexes and Performance
==============================================

In this tutorial, you'll learn how to dramatically improve query performance using LearnQL's seamless secondary index API. You'll create indexes with the fluent interface, perform fast lookups with ``find_by`` and ``find_all_by``, execute range queries, and understand how B-tree indexes accelerate database operations.

**Time**: 30 minutes
**Level**: Intermediate
**Prerequisites**: Completed :doc:`tutorial-05-aggregations-groupby`

What We'll Build
----------------

A high-performance student database with:

* Fluent API for declaring indexes with ``.add_index()``
* Unique indexes for one-to-one relationships (name → student)
* Multi-value indexes for one-to-many relationships (department → students)
* Fast O(log n) lookups with ``find_by()`` and ``find_all_by()``
* Range queries for numeric and string fields
* Automatic CRUD synchronization (indexes auto-update)
* Persistent indexes that survive database restarts

Understanding Indexes
---------------------

What Is an Index?
~~~~~~~~~~~~~~~~~

An index is a data structure that speeds up data retrieval. Think of it like a book's index: instead of reading every page, you jump directly to the right page.

.. code-block:: text

   Without Index (Full Table Scan):
   O(n) - Check every record

   ┌─────┬─────┬─────┬─────┬─────┬─────┐
   │ R1  │ R2  │ R3  │ R4  │ R5  │ R6  │ ← Scan all
   └─────┴─────┴─────┴─────┴─────┴─────┘

   With Index (B-tree):
   O(log n) - Tree traversal

            ┌──────┐
            │  M   │         ← Root node
            └──┬───┘
        ┌──────┴──────┐
      ┌─┴─┐         ┌─┴─┐   ← Internal nodes
      │ F │         │ S │
      └───┘         └───┘
      ↓             ↓       ← Point to data
    Records       Records

B-Tree Performance
~~~~~~~~~~~~~~~~~~

.. note::
   For 1 million records:

   * **Full scan**: 1,000,000 comparisons
   * **B-tree index**: ~20 comparisons (log₂ 1M ≈ 20)
   * **Speedup**: 50,000x faster!

Index Types
~~~~~~~~~~~

* **Unique Index**: Enforces uniqueness (one value → one record)
* **Multi-Value Index**: Allows duplicates (one value → many records)

Step 1: Define the Student Model
---------------------------------

Create ``indexed_students.cpp``:

.. code-block:: cpp

   #include "learnql/LearnQL.hpp"
   #include <iostream>
   #include <string>
   #include <iomanip>

   using namespace learnql;

   struct Student {
       LEARNQL_PROPERTY(int, student_id);
       LEARNQL_PROPERTY(std::string, name);
       LEARNQL_PROPERTY(std::string, department);
       LEARNQL_PROPERTY(int, age);
       LEARNQL_PROPERTY(double, gpa);
   };

**Fields to index:**

* ``name``: Unique index (one name → one student)
* ``department``: Multi-value index (one dept → many students)
* ``gpa``: Multi-value index (for range queries)

Step 2: Creating Indexes with Fluent API
-----------------------------------------

LearnQL's fluent API makes index declaration clean and chainable.

Declare Indexes
~~~~~~~~~~~~~~~

.. code-block:: cpp

   int main() {
       try {
           Database db("university.db");

           // Create table with indexes using fluent API
           auto& students = db.table<Student>("students")
               .add_index(Student::name, core::IndexType::Unique)
               .add_index(Student::department, core::IndexType::MultiValue)
               .add_index(Student::gpa, core::IndexType::MultiValue);

           std::cout << "✓ Table created with 3 secondary indexes:\n";
           std::cout << "  - name (Unique)\n";
           std::cout << "  - department (MultiValue)\n";
           std::cout << "  - gpa (MultiValue)\n\n";

**How it works:**

1. ``db.table<Student>("students")`` returns a ``Table&``
2. ``.add_index()`` adds an index and returns ``Table&`` for chaining
3. Indexes are automatically registered in the system catalog
4. If the table exists, indexes are loaded from disk

Index Types Explained
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Unique Index: Enforces uniqueness
   .add_index(Student::name, core::IndexType::Unique)
   // Fast lookup: find_by(Student::name, "Alice")
   // Returns: std::optional<Student> (one or none)

   // Multi-Value Index: Allows duplicates
   .add_index(Student::department, core::IndexType::MultiValue)
   // Fast lookup: find_all_by(Student::department, "CS")
   // Returns: std::vector<Student> (zero or more)

Step 3: Populating Data with Auto-Index Updates
------------------------------------------------

Indexes automatically update during INSERT operations.

.. code-block:: cpp

   void populate_students(Table<Student>& students) {
       std::cout << "Inserting students (indexes auto-update)...\n";

       students.insert({1001, "Alice Johnson", "CS", 20, 3.8});
       students.insert({1002, "Bob Smith", "Math", 21, 3.5});
       students.insert({1003, "Carol White", "CS", 19, 3.9});
       students.insert({1004, "David Brown", "Physics", 22, 3.6});
       students.insert({1005, "Eve Davis", "CS", 20, 3.7});
       students.insert({1006, "Frank Wilson", "Math", 23, 3.4});
       students.insert({1007, "Grace Lee", "CS", 19, 4.0});
       students.insert({1008, "Henry Taylor", "Physics", 21, 3.8});

       std::cout << "✓ Inserted " << students.size() << " students\n";
       std::cout << "✓ All indexes automatically updated!\n\n";
   }

.. note::
   You don't need to manually update indexes. LearnQL synchronizes all indexes automatically during INSERT, UPDATE, and DELETE operations.

Step 4: Fast Lookups with find_by() - Unique Index
---------------------------------------------------

Use ``find_by()`` for unique index lookups.

Find Student by Name
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void find_by_name_demo(Table<Student>& students) {
       std::cout << "=== Finding Student by Name (Unique Index) ===\n";
       std::cout << std::string(60, '-') << "\n";

       std::string search_name = "Alice Johnson";
       std::cout << "Searching for: " << search_name << "\n";

       auto alice = students.find_by(Student::name, search_name);

       if (alice) {
           std::cout << "✓ Found!\n";
           std::cout << "  ID: " << alice->get_student_id() << "\n";
           std::cout << "  Name: " << alice->get_name() << "\n";
           std::cout << "  Department: " << alice->get_department() << "\n";
           std::cout << "  GPA: " << std::fixed << std::setprecision(2)
                     << alice->get_gpa() << "\n";
           std::cout << "\nPerformance: O(log n) index lookup!\n";
       } else {
           std::cout << "✗ Student not found\n";
       }
   }

**API signature:**

.. code-block:: cpp

   std::optional<Student> find_by(
       Field field,               // Static field (Student::name)
       const FieldType& value     // Value to search for
   );

**Returns:**

* ``std::optional<Student>`` - Contains student if found, empty otherwise
* Uses B-tree index for O(log n) lookup

**Expected output:**

.. code-block:: text

   Searching for: Alice Johnson
   ✓ Found!
     ID: 1001
     Name: Alice Johnson
     Department: CS
     GPA: 3.80

   Performance: O(log n) index lookup!

Step 5: Multi-Value Lookups with find_all_by()
-----------------------------------------------

Use ``find_all_by()`` for multi-value index lookups.

Find All Students in Department
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void find_all_by_department_demo(Table<Student>& students) {
       std::cout << "\n=== Finding All CS Students (Multi-Value Index) ===\n";
       std::cout << std::string(60, '-') << "\n";

       auto cs_students = students.find_all_by(Student::department,
                                               std::string("CS"));

       std::cout << "✓ Found " << cs_students.size() << " CS students:\n";
       for (const auto& student : cs_students) {
           std::cout << "  - " << std::setw(20) << std::left
                     << student.get_name()
                     << " (GPA: " << std::fixed << std::setprecision(2)
                     << student.get_gpa() << ")\n";
       }
       std::cout << "\nPerformance: O(log n + k) where k = result count\n";
   }

**API signature:**

.. code-block:: cpp

   std::vector<Student> find_all_by(
       Field field,               // Static field (Student::department)
       const FieldType& value     // Value to search for
   );

**Returns:**

* ``std::vector<Student>`` - All students matching the value
* Empty vector if no matches
* Uses B-tree to find first match, then traverses adjacent nodes

**Expected output:**

.. code-block:: text

   ✓ Found 4 CS students:
     - Alice Johnson       (GPA: 3.80)
     - Carol White         (GPA: 3.90)
     - Eve Davis           (GPA: 3.70)
     - Grace Lee           (GPA: 4.00)

   Performance: O(log n + k) where k = result count

Step 6: Range Queries
---------------------

Find all records where a field falls within a range.

Find Students in GPA Range
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void range_query_demo(Table<Student>& students) {
       std::cout << "\n=== Range Query: GPA between 3.5 and 4.0 ===\n";
       std::cout << std::string(60, '-') << "\n";

       auto high_gpa_students = students.range_query(Student::gpa, 3.5, 4.0);

       std::cout << "✓ Found " << high_gpa_students.size()
                 << " high-GPA students:\n";

       for (const auto& student : high_gpa_students) {
           std::cout << "  - " << std::setw(20) << std::left
                     << student.get_name()
                     << " GPA: " << std::fixed << std::setprecision(2)
                     << student.get_gpa() << "\n";
       }
   }

**API signature:**

.. code-block:: cpp

   std::vector<Student> range_query(
       Field field,
       const FieldType& min_value,
       const FieldType& max_value
   );

**How it works:**

1. Uses the B-tree index to find the first value ≥ min_value
2. Traverses leaf nodes collecting values ≤ max_value
3. Returns all matching records

**Expected output:**

.. code-block:: text

   ✓ Found 6 high-GPA students:
     - Alice Johnson       GPA: 3.80
     - Bob Smith           GPA: 3.50
     - Carol White         GPA: 3.90
     - David Brown         GPA: 3.60
     - Eve Davis           GPA: 3.70
     - Grace Lee           GPA: 4.00

Step 7: Getting Unique Values
------------------------------

Extract all unique values from an indexed field.

.. code-block:: cpp

   void unique_values_demo(Table<Student>& students) {
       std::cout << "\n=== Unique Departments ===\n";
       std::cout << std::string(60, '-') << "\n";

       auto departments = students.get_unique_values(Student::department);

       std::cout << "✓ Found " << departments.size() << " departments:\n";
       for (const auto& dept : departments) {
           auto dept_students = students.find_all_by(Student::department, dept);
           std::cout << "  - " << std::setw(12) << std::left << dept
                     << " (" << dept_students.size() << " students)\n";
       }
   }

**API signature:**

.. code-block:: cpp

   std::vector<FieldType> get_unique_values(Field field);

**Expected output:**

.. code-block:: text

   ✓ Found 3 departments:
     - CS          (4 students)
     - Math        (2 students)
     - Physics     (2 students)

Step 8: Automatic CRUD Synchronization
---------------------------------------

Indexes automatically stay synchronized during updates and deletes.

Update Example
~~~~~~~~~~~~~~

.. code-block:: cpp

   void update_demo(Table<Student>& students) {
       std::cout << "\n=== Automatic Index Synchronization ===\n";
       std::cout << std::string(60, '-') << "\n";

       // Find Alice
       auto alice = students.find_by(Student::name, std::string("Alice Johnson"));
       if (!alice) return;

       std::cout << "Before update:\n";
       std::cout << "  " << alice->get_name() << " is in "
                 << alice->get_department() << "\n";

       // Update department
       alice->set_department("Engineering");
       students.update(*alice);

       std::cout << "\nAfter update:\n";
       auto alice_updated = students.find_by(Student::name,
                                             std::string("Alice Johnson"));
       if (alice_updated) {
           std::cout << "  " << alice_updated->get_name() << " is in "
                     << alice_updated->get_department() << "\n";
       }

       // Verify Engineering department index
       auto eng_students = students.find_all_by(Student::department,
                                                std::string("Engineering"));
       std::cout << "\nEngineering department now has "
                 << eng_students.size() << " students\n";

       std::cout << "✓ All indexes automatically synchronized!\n";

       // Restore original state
       alice_updated->set_department("CS");
       students.update(*alice_updated);
   }

**What happens during update:**

1. Old index entries are removed (``name`` index, old ``department`` value)
2. New index entries are added (``name`` index, new ``department`` value)
3. All indexes stay consistent automatically

Step 9: Index Persistence
--------------------------

Indexes are persistent and survive database restarts.

.. code-block:: cpp

   void persistence_demo(Database& db, Table<Student>& students) {
       std::cout << "\n=== Index Persistence ===\n";
       std::cout << std::string(60, '-') << "\n";

       std::cout << "Flushing database...\n";
       students.flush();
       std::cout << "✓ Indexes persisted to disk\n\n";

       std::cout << "Simulating restart: Reopening table...\n";
       auto& reloaded = db.table<Student>("students")
           .add_index(Student::name, core::IndexType::Unique)
           .add_index(Student::department, core::IndexType::MultiValue)
           .add_index(Student::gpa, core::IndexType::MultiValue);

       std::cout << "✓ Table and indexes reloaded from disk\n";
       std::cout << "  Indexes detected existing data and loaded automatically!\n\n";

       // Verify index still works
       auto alice = reloaded.find_by(Student::name, std::string("Alice Johnson"));
       if (alice) {
           std::cout << "✓ Index lookup still works after reload:\n";
           std::cout << "  Found: " << alice->get_name() << "\n";
       }
   }

Step 10: Performance Comparison
--------------------------------

Compare full table scan vs. indexed lookup.

.. code-block:: cpp

   void performance_comparison(Table<Student>& students) {
       std::cout << "\n=== Performance Comparison ===\n";
       std::cout << std::string(80, '=') << "\n";

       std::cout << "\nScenario: Find all CS students\n\n";

       // Method 1: Full table scan
       std::cout << "1. Full Table Scan (no index):\n";
       std::cout << "   Complexity: O(n)\n";
       std::cout << "   Must check ALL " << students.size() << " records\n";
       std::cout << "   For 1M records: ~1,000,000 comparisons\n\n";

       // Method 2: Indexed lookup
       std::cout << "2. find_all_by with Department Index:\n";
       std::cout << "   Complexity: O(log n + k) where k = result count\n";
       std::cout << "   B-tree traversal: ~" << static_cast<int>(std::log2(students.size()))
                 << " comparisons\n";
       std::cout << "   For 1M records: ~20 comparisons + retrieve results\n";
       std::cout << "   Speedup: ~50,000x faster!\n\n";

       std::cout << std::string(80, '=') << "\n";
   }

Step 11: Complete Program
--------------------------

.. code-block:: cpp

   int main() {
       try {
           Database db("university.db");

           // Create table with indexes
           auto& students = db.table<Student>("students")
               .add_index(Student::name, core::IndexType::Unique)
               .add_index(Student::department, core::IndexType::MultiValue)
               .add_index(Student::gpa, core::IndexType::MultiValue);

           // Populate if empty
           if (students.size() == 0) {
               populate_students(students);
           }

           // Demonstrate index features
           find_by_name_demo(students);
           find_all_by_department_demo(students);
           range_query_demo(students);
           unique_values_demo(students);
           update_demo(students);
           persistence_demo(db, students);
           performance_comparison(students);

           return 0;
       } catch (const std::exception& e) {
           std::cerr << "Error: " << e.what() << std::endl;
           return 1;
       }
   }

Index API Summary
-----------------

Creating Indexes
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Fluent API with chaining
   auto& table = db.table<Record>("table_name")
       .add_index(Record::field1, core::IndexType::Unique)
       .add_index(Record::field2, core::IndexType::MultiValue);

Lookup Operations
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Unique index: returns std::optional
   auto record = table.find_by(Record::unique_field, value);

   // Multi-value index: returns std::vector
   auto records = table.find_all_by(Record::multi_field, value);

   // Range query: returns std::vector
   auto range = table.range_query(Record::numeric_field, min, max);

   // Get unique values: returns std::vector
   auto unique = table.get_unique_values(Record::field);

CRUD Operations
~~~~~~~~~~~~~~~

.. code-block:: cpp

   // All automatically update indexes
   table.insert(record);    // Adds to all indexes
   table.update(record);    // Updates all indexes
   table.remove(record);    // Removes from all indexes

Index Types
~~~~~~~~~~~

* **Unique**: One value → one record (use for IDs, usernames, emails)
* **MultiValue**: One value → many records (use for categories, status, departments)

What You Learned
----------------

✅ Creating indexes with fluent API (``.add_index()``)

✅ Unique indexes vs. multi-value indexes

✅ Fast lookups with ``find_by()`` and ``find_all_by()``

✅ Range queries on numeric and string fields

✅ Getting unique values from indexed fields

✅ Automatic CRUD synchronization

✅ Index persistence across database restarts

✅ B-tree performance characteristics (O(log n))

✅ Understanding when to create indexes

✅ 50,000x speedup with proper indexing

Exercises
---------

Try these challenges to master indexes:

1. **Email unique index**

   Add an email field to Student and create a unique index. Implement a "find by email" function.

2. **Age range statistics**

   Create an index on age. Write queries to find students in different age ranges (18-20, 21-23, etc.).

3. **Composite index simulation**

   Manually implement a composite index on (department, gpa) using a map with pair keys. Compare performance.

4. **Index selectivity analyzer**

   Write a function that analyzes field selectivity (unique values / total records) to recommend which fields should be indexed.

5. **Benchmark tool**

   Create a benchmark tool that measures query performance with and without indexes for different table sizes.

Next Steps
----------

Continue to :doc:`tutorial-07-advanced-features` to learn about C++20 ranges integration, lambda predicates with ``find_if``, and advanced query patterns.

.. tip::
   Create indexes on fields you frequently query in WHERE clauses. For read-heavy workloads, more indexes improve performance. For write-heavy workloads, fewer indexes reduce overhead. Always measure performance to validate your indexing strategy!
