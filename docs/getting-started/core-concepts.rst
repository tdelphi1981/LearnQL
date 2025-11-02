Core Concepts
=============

This guide explains the fundamental concepts of LearnQL, designed to help C++ beginners understand how the library works and how to use it effectively.

The LearnQL Philosophy
----------------------

LearnQL is built around three core principles:

1. **Type Safety**: Catch errors at compile-time, not runtime
2. **Zero-Cost Abstraction**: High-level syntax with no performance penalty
3. **Educational Value**: Learn modern C++20 while building databases

Database Architecture
---------------------

Understanding the Stack
~~~~~~~~~~~~~~~~~~~~~~~

LearnQL has a layered architecture:

.. code-block:: text

   ┌─────────────────────────────────────┐
   │   Your Application Code             │
   ├─────────────────────────────────────┤
   │   Query DSL (SQL-like expressions)  │ ← Type-safe queries
   ├─────────────────────────────────────┤
   │   Table API (CRUD operations)       │ ← High-level interface
   ├─────────────────────────────────────┤
   │   Indexing (B-tree, Secondary)      │ ← Fast lookups
   ├─────────────────────────────────────┤
   │   Serialization Layer               │ ← Object ↔ Bytes
   ├─────────────────────────────────────┤
   │   Storage Engine (Page-based)       │ ← Persistent storage
   └─────────────────────────────────────┘

Key Components
~~~~~~~~~~~~~~

**Database**
   The top-level container that manages tables and storage. One database = one file.

**Table**
   A collection of records of the same type. Tables are type-safe - you can't insert the wrong type!

**Record**
   A single instance of your class stored in the database.

**RecordId**
   An internal identifier for a record's physical location in storage.

**Primary Key**
   The field marked with ``PK`` in your ``LEARNQL_PROPERTY`` macro. Must be unique and is automatically indexed.

The Property System
-------------------

What Are Properties?
~~~~~~~~~~~~~~~~~~~~

In LearnQL, properties are fields that the database knows about. The ``LEARNQL_PROPERTIES`` macro system transforms a simple class into a full-featured database entity.

Before LearnQL
^^^^^^^^^^^^^^

Traditional C++ struct (not database-aware):

.. code-block:: cpp

   struct Student {
       int id;
       std::string name;
       int age;
   };

   // Manual serialization needed
   // No type information available at compile-time
   // No reflection capabilities
   // No query support

With LearnQL
^^^^^^^^^^^^

Database-aware class using the property system:

.. code-block:: cpp

   class Student {
       LEARNQL_PROPERTIES_BEGIN(Student)
           LEARNQL_PROPERTY(int, student_id, PK)
           LEARNQL_PROPERTY(std::string, name)
           LEARNQL_PROPERTY(int, age)
       LEARNQL_PROPERTIES_END(
           PROP(int, student_id, PK),
           PROP(std::string, name),
           PROP(int, age)
       )

   public:
       Student() = default;
   };

   // ✓ Automatic serialization
   // ✓ Compile-time type information
   // ✓ Reflection support
   // ✓ Query DSL integration
   // ✓ Getter/setter methods

What the Macro Generates
~~~~~~~~~~~~~~~~~~~~~~~~~

The ``LEARNQL_PROPERTIES`` macro system expands to generate:

1. **Private storage** for each property (with trailing underscore: ``student_id_``, ``name_``, ``age_``)
2. **Getter methods** (``get_student_id()``, ``get_name()``, ``get_age()``)
3. **Setter methods** (``set_student_id()``, ``set_name()``, ``set_age()``)
4. **Static Field objects** for queries (``Student::student_id``, ``Student::name``, ``Student::age``)
5. **Type metadata** for reflection
6. **Serialization support** for persistence

Example property access:

.. code-block:: cpp

   Student student;

   // Use setters to modify
   student.set_student_id(1001);
   student.set_name("Alice Johnson");
   student.set_age(20);

   // Use getters to read
   std::cout << "ID: " << student.get_student_id() << std::endl;
   std::cout << "Name: " << student.get_name() << std::endl;
   std::cout << "Age: " << student.get_age() << std::endl;

   // Or use convenience constructor
   Student bob(1002, "Bob Smith", 22);

.. note::
   You don't need to understand the macro expansion details to use LearnQL. The macro handles everything automatically!

Supported Types
~~~~~~~~~~~~~~~

LearnQL properties support common C++ types:

**Primitive Types**
   ``int``, ``long``, ``float``, ``double``, ``bool``, ``char``

**String Types**
   ``std::string``

**Optional Types**
   ``std::optional<T>`` for nullable fields (not shown in main.cpp but supported)

Example with various types:

.. code-block:: cpp

   class Person {
       LEARNQL_PROPERTIES_BEGIN(Person)
           LEARNQL_PROPERTY(int, person_id, PK)
           LEARNQL_PROPERTY(std::string, name)
           LEARNQL_PROPERTY(double, salary)
           LEARNQL_PROPERTY(bool, is_active)
       LEARNQL_PROPERTIES_END(
           PROP(int, person_id, PK),
           PROP(std::string, name),
           PROP(double, salary),
           PROP(bool, is_active)
       )

   public:
       Person() = default;
   };

Type Safety and Expression Templates
-------------------------------------

Static Field Objects
~~~~~~~~~~~~~~~~~~~~

When you define properties, LearnQL automatically generates static Field objects:

.. code-block:: cpp

   class Student {
       LEARNQL_PROPERTIES_BEGIN(Student)
           LEARNQL_PROPERTY(int, student_id, PK)
           LEARNQL_PROPERTY(std::string, name)
           LEARNQL_PROPERTY(int, age)
           LEARNQL_PROPERTY(double, gpa)
       LEARNQL_PROPERTIES_END(
           PROP(int, student_id, PK),
           PROP(std::string, name),
           PROP(int, age),
           PROP(double, gpa)
       )
   };

   // These static Field objects are generated automatically:
   // Student::student_id (type: Field<Student, int>)
   // Student::name (type: Field<Student, std::string>)
   // Student::age (type: Field<Student, int>)
   // Student::gpa (type: Field<Student, double>)

Using Static Fields in Queries
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You use these static Field objects directly in query expressions:

.. code-block:: cpp

   // Simple comparison
   auto adults = students.where(Student::age >= 21);

   // String equality
   auto cs_students = students.where(Student::department == "CS");

   // Combined with AND
   auto elite_cs = students.where(
       (Student::department == "CS") && (Student::gpa >= 3.7)
   );

   // Combined with OR
   auto young_or_smart = students.where(
       (Student::age <= 20) || (Student::gpa >= 3.9)
   );

How Expression Templates Work
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1. **``Student::age`` is a static Field object** containing metadata about the age property
2. **``>= 21`` creates a comparison expression** that's evaluated at compile-time
3. The entire expression is **type-checked during compilation**
4. **Zero runtime overhead** - the expression compiles down to efficient code

.. code-block:: cpp

   Student::age >= 21
   // ↓
   // Becomes: BinaryExpression<Field<Student, int>, GreaterEqual, Constant<21>>
   // Type-checked and optimized at compile-time!

Compile-Time Type Checking
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Invalid expressions are caught at compile-time:

.. code-block:: cpp

   // ✓ Valid: comparing int with int
   students.where(Student::age >= 21);

   // ✗ Compile error: can't compare string with int
   students.where(Student::name >= 21);  // Error!

   // ✗ Compile error: no such field
   students.where(Student::invalid_field == 5);  // Error!

This is **zero-cost abstraction**: the type checking happens during compilation, so there's no runtime penalty!

Storage and Persistence
------------------------

How Data is Stored
~~~~~~~~~~~~~~~~~~

LearnQL uses a **page-based storage engine** similar to real databases:

.. code-block:: text

   Database File (university.db)
   ┌──────────────────────────────────┐
   │ Page 0: Metadata                 │
   ├──────────────────────────────────┤
   │ Page 1: Students table - Page 1  │
   │   Record 1: {1, "Alice", 20}     │
   │   Record 2: {2, "Bob", 22}       │
   ├──────────────────────────────────┤
   │ Page 2: Students table - Page 2  │
   │   Record 3: {3, "Carol", 19}     │
   │   ...                            │
   ├──────────────────────────────────┤
   │ Page 3: B-tree index for ID      │
   └──────────────────────────────────┘

Pages
~~~~~

* **Fixed size** (default: 4096 bytes)
* **Unit of I/O**: LearnQL reads/writes entire pages
* **Efficient**: Similar to operating system page caching

Record Layout
~~~~~~~~~~~~~

Records are serialized to bytes:

.. code-block:: cpp

   Student{1, "Alice", 20}
   // Serializes to:
   // [4 bytes: id=1][8 bytes: string length=5][5 bytes: "Alice"][4 bytes: age=20]

Indexing
--------

Primary Key Index
~~~~~~~~~~~~~~~~~

The field marked with ``PK`` is automatically used as the primary key and indexed with a B-tree:

.. code-block:: cpp

   class Student {
       LEARNQL_PROPERTIES_BEGIN(Student)
           LEARNQL_PROPERTY(int, student_id, PK)  // ← Primary key (auto-indexed)
           LEARNQL_PROPERTY(std::string, name)
           LEARNQL_PROPERTY(int, age)
       LEARNQL_PROPERTIES_END(
           PROP(int, student_id, PK),
           PROP(std::string, name),
           PROP(int, age)
       )
   };

   // Fast lookup by primary key
   auto student = students.find(123);  // O(log n)

Why B-trees?
^^^^^^^^^^^^

B-trees provide:

* **O(log n) lookups** - Fast even with millions of records
* **Sorted order** - Range queries are efficient
* **Disk-friendly** - Minimizes I/O operations

Secondary Indexes
~~~~~~~~~~~~~~~~~

You can create indexes on other fields for fast non-primary-key lookups:

.. code-block:: cpp

   // Create a secondary index using fluent API
   auto& students = db.table<Student>("students")
       .add_index(Student::department, core::IndexType::MultiValue);

   // Fast lookup by department (O(log n) instead of O(n))
   auto cs_students = students.find_all_by(Student::department, std::string("CS"));

   // Range queries
   auto high_gpa = students.range_query(Student::gpa, 3.5, 4.0);

   // Get unique values
   auto departments = students.get_unique_values(Student::department);

Learn more in :doc:`/tutorials/tutorial-06-indexes-performance`.

CRUD Operations
---------------

Create (Insert)
~~~~~~~~~~~~~~~

.. code-block:: cpp

   students.insert(Student(1, "Alice", 20));
   // 1. Serialize record to bytes
   // 2. Find free space in a page
   // 3. Write to storage
   // 4. Update primary key index
   // 5. Update any secondary indexes

Read (Query)
~~~~~~~~~~~~

.. code-block:: cpp

   // Iterate all records
   for (const auto& student : students) {
       std::cout << student.get_name() << std::endl;
   }

   // Filter using expression templates
   auto results = students.where(Student::age >= 21);
   for (const auto& student : results) {
       std::cout << student.get_name() << std::endl;
   }

   // Find by primary key
   auto student = students.find(1001);
   if (student) {
       std::cout << "Found: " << student->get_name() << std::endl;
   }

Update
~~~~~~

.. code-block:: cpp

   auto student = students.find(1001);
   if (student) {
       student->set_age(21);  // Use setter
       students.update(*student);
   }
   // 1. Find record using index
   // 2. Serialize updated record
   // 3. Write to storage
   // 4. Update any secondary indexes

Delete (Remove)
~~~~~~~~~~~~~~~

.. code-block:: cpp

   students.remove(1001);  // Remove by primary key
   // 1. Remove from primary key index
   // 2. Remove from secondary indexes (if any)
   // 3. Mark page space as free

Query Execution Model
---------------------

Direct Iteration
~~~~~~~~~~~~~~~~

LearnQL queries return iterable ranges directly:

.. code-block:: cpp

   // where() returns an iterable range
   auto results = students.where(Student::age >= 21);

   // Iterate immediately
   for (const auto& student : results) {
       std::cout << student.get_name() << std::endl;
   }

Batched Loading
~~~~~~~~~~~~~~~

LearnQL uses batched loading for memory efficiency:

.. code-block:: cpp

   // Tables use batch size of 10 by default
   auto& students = db.table<Student>("students");

   // Only 10 records loaded into memory at a time
   for (const auto& student : students) {
       // Process student
       // Old batches automatically discarded
   }

.. note::
   With batched loading, memory usage stays constant regardless of table size. Perfect for processing millions of records!

Iterator-Based Results
~~~~~~~~~~~~~~~~~~~~~~

Results are returned as iterators for memory efficiency:

.. code-block:: cpp

   auto results = students.where(Student::gpa > 3.5);

   // Results are iterable
   for (const auto& student : results) {
       // Each record is deserialized on-demand
       std::cout << student.get_name() << std::endl;
   }

Memory Management
-----------------

Stack vs. Heap
~~~~~~~~~~~~~~

**Most LearnQL objects live on the stack**, which is fast and automatic:

.. code-block:: cpp

   core::Database db("file.db");         // Stack allocated
   auto& students = db.table<Student>(); // Reference (no allocation)
   auto results = students.where(...);   // Stack allocated

**Storage pages use the heap** for larger data:

.. code-block:: cpp

   students.insert({...});  // Data written to heap-allocated pages

RAII (Resource Acquisition Is Initialization)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

LearnQL uses RAII for automatic cleanup:

.. code-block:: cpp

   {
       core::Database db("file.db");  // File opened
       auto& table = db.table<Student>("students");
       // Use the database...
   }  // ← Database automatically flushed and closed

No manual cleanup needed!

Performance Characteristics
---------------------------

Time Complexity
~~~~~~~~~~~~~~~

+------------------------+------------------+------------------------+
| Operation              | Best Case        | Worst Case             |
+========================+==================+========================+
| Insert                 | O(log n)         | O(log n)               |
+------------------------+------------------+------------------------+
| Find by primary key    | O(log n)         | O(log n)               |
+------------------------+------------------+------------------------+
| Find by secondary idx  | O(log n)         | O(log n)               |
+------------------------+------------------+------------------------+
| where() (full scan)    | O(n)             | O(n)                   |
+------------------------+------------------+------------------------+
| where() with index     | O(log n + k)     | O(log n + k)*          |
+------------------------+------------------+------------------------+
| Update                 | O(log n)         | O(log n)               |
+------------------------+------------------+------------------------+
| Delete                 | O(log n)         | O(log n)               |
+------------------------+------------------+------------------------+

*k = number of matching records

Space Complexity
~~~~~~~~~~~~~~~~

* **Overhead per record**: ~16-24 bytes (metadata)
* **Index overhead**: ~16 bytes per entry
* **Page overhead**: ~64 bytes per page
* **Memory usage**: Constant with batched loading (default batch size: 10)

When to Use LearnQL
-------------------

**Good For** ✅

* Educational projects and learning C++20
* Prototyping data-driven applications
* Small to medium datasets (< 1 million records)
* Single-threaded applications
* Embedded databases

**Not Suitable For** ❌

* Production systems requiring ACID guarantees
* Multi-threaded concurrent access
* Extremely large datasets (> millions of records)
* Applications requiring SQL compliance
* Distributed systems

Next Steps
----------

Now that you understand the core concepts:

* :doc:`property-macros` - Advanced property techniques
* :doc:`/tutorials/tutorial-01-first-database` - Build your first real application
* :doc:`/architecture/overview` - Deep dive into the architecture
* :doc:`/api/core` - Explore the Core API reference

.. tip::
   Don't worry if not everything makes sense yet! The concepts will become clearer as you work through the tutorials and build real applications.
