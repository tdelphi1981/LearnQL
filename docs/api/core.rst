Core Module
===========

The Core module provides the fundamental classes for working with LearnQL: ``Database`` and ``Table``. These are the primary interfaces you'll interact with for managing your database and performing CRUD operations.

Overview
--------

The Core module consists of two main components:

* **Database**: Top-level container managing storage and tables
* **Table**: Type-safe interface for working with records

.. mermaid::

   graph TB
       A[Database] --> B[Table<T>]
       B --> C[Storage Engine]
       B --> D[Index System]
       B --> E[System Catalog]

Database Class
--------------

The ``Database`` class is the entry point to LearnQL. It manages file I/O, storage pages, and table instances.

Class Reference
~~~~~~~~~~~~~~~

.. doxygenclass:: learnql::core::Database
   :members:
   :undoc-members:

Constructor
~~~~~~~~~~~

.. code-block:: cpp

   Database(const std::string& filename);

**Parameters:**

* ``filename``: Path to the database file

**Example:**

.. code-block:: cpp

   // File-based database
   core::Database db("university.db");

**Throws:**

* ``std::runtime_error``: If the file cannot be opened or created

table() Method
~~~~~~~~~~~~~~

.. code-block:: cpp

   template<typename T>
   Table<T>& table(const std::string& name);

Creates or opens a table for type ``T``.

**Parameters:**

* ``name``: Name of the table

**Returns:** Reference to the ``Table<T>`` instance

**Example:**

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
   };

   core::Database db("university.db");
   auto& students = db.table<Student>("students");

**Important:**

* The table is created if it doesn't exist
* The returned reference is valid for the lifetime of the Database object
* Table metadata is automatically registered in the system catalog

metadata() Method
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   catalog::SystemCatalog& metadata();

Returns the system catalog for querying database metadata.

**Returns:** Reference to ``SystemCatalog``

**Example:**

.. code-block:: cpp

   auto& catalog = db.metadata();
   auto tables = catalog.tables().get_all();

flush() Method
~~~~~~~~~~~~~~

.. code-block:: cpp

   void flush();

Persists all pending changes to disk.

**Example:**

.. code-block:: cpp

   db.flush();  // Flushes all tables and storage

**Note:** Database automatically flushes when destroyed.

Usage Example
~~~~~~~~~~~~~

.. code-block:: cpp

   #include <learnql/LearnQL.hpp>

   using namespace learnql;

   class Person {
       LEARNQL_PROPERTIES_BEGIN(Person)
           LEARNQL_PROPERTY(int, id, PK)
           LEARNQL_PROPERTY(std::string, name)
       LEARNQL_PROPERTIES_END(
           PROP(int, id, PK),
           PROP(std::string, name)
       )
   };

   int main() {
       // Create database
       core::Database db("people.db");

       // Get table reference
       auto& people = db.table<Person>("people");

       // Use the table
       people.insert(Person{1, "Alice"});

       // Database automatically saved when db goes out of scope
       return 0;
   }

Table Class
-----------

The ``Table<T>`` class provides type-safe CRUD operations and query building.

Class Reference
~~~~~~~~~~~~~~~

.. doxygenclass:: learnql::core::Table
   :members:
   :undoc-members:

Insert Operations
~~~~~~~~~~~~~~~~~

insert()
^^^^^^^^

.. code-block:: cpp

   void insert(const T& record);

Inserts a new record into the table.

**Parameters:**

* ``record``: The record to insert

**Throws:**

* ``std::runtime_error``: If a record with the same primary key already exists

**Example:**

.. code-block:: cpp

   Student student{1001, "Alice Johnson", 20};
   students.insert(student);

   // Or using aggregate initialization
   students.insert({1002, "Bob Smith", 22});

**Time Complexity:** O(log n) due to index update

Query Operations
~~~~~~~~~~~~~~~~

find()
^^^^^^

.. code-block:: cpp

   std::optional<T> find(const KeyType& primary_key);

Finds a record by its primary key.

**Parameters:**

* ``primary_key``: The primary key value to search for

**Returns:** ``std::optional<T>`` containing the record if found, or ``std::nullopt`` if not found

**Example:**

.. code-block:: cpp

   auto student = students.find(1001);

   if (student) {
       std::cout << "Found: " << student->get_name() << std::endl;
   } else {
       std::cout << "Student not found" << std::endl;
   }

**Time Complexity:** O(log n) using B-tree index

where()
^^^^^^^

.. code-block:: cpp

   template<typename ExprType>
   auto where(const ExprType& expr);

Filters records using an expression template.

**Parameters:**

* ``expr``: Expression to filter records (built using Field objects and operators)

**Returns:** Iterable range of matching records

**Example:**

.. code-block:: cpp

   // Simple comparison
   auto adults = students.where(Student::age >= 18);

   // Combined conditions
   auto honor_students = students.where(
       (Student::age >= 18) && (Student::gpa >= 3.5)
   );

   // Iterate results
   for (const auto& student : adults) {
       std::cout << student.get_name() << std::endl;
   }

**Supported Operators:**

* Comparison: ``==``, ``!=``, ``<``, ``<=``, ``>``, ``>=``
* Logical: ``&&`` (AND), ``||`` (OR)

find_if()
^^^^^^^^^

.. code-block:: cpp

   template<typename Predicate>
   auto find_if(Predicate pred);

Finds records matching a lambda predicate.

**Parameters:**

* ``pred``: Lambda function returning ``bool``

**Returns:** Iterable range of matching records

**Example:**

.. code-block:: cpp

   auto cs_students = students.find_if([](const Student& s) {
       return s.get_department() == "CS" && s.get_gpa() > 3.5;
   });

   for (const auto& student : cs_students) {
       std::cout << student.get_name() << std::endl;
   }

get_all()
^^^^^^^^^

.. code-block:: cpp

   auto get_all() const;

Returns an iterator over all records in the table.

**Returns:** Range object that can be iterated

**Example:**

.. code-block:: cpp

   for (const auto& student : students.get_all()) {
       std::cout << student.get_name() << std::endl;
   }

   // Or using simplified iteration
   for (const auto& student : students) {
       std::cout << student.get_name() << std::endl;
   }

Update Operations
~~~~~~~~~~~~~~~~~

update()
^^^^^^^^

.. code-block:: cpp

   void update(const T& record);

Updates an existing record (matched by primary key).

**Parameters:**

* ``record``: The record with updated values

**Throws:**

* ``std::runtime_error``: If the record doesn't exist

**Example:**

.. code-block:: cpp

   auto student = students.find(1001);
   if (student) {
       student->set_gpa(3.9);  // Using setter from LEARNQL_PROPERTY
       students.update(*student);
   }

**Important:** The primary key in the record is used to locate which record to update. Do not change the primary key value.

Delete Operations
~~~~~~~~~~~~~~~~~

remove()
^^^^^^^^

.. code-block:: cpp

   void remove(const KeyType& primary_key);

Deletes a record from the table by primary key.

**Parameters:**

* ``primary_key``: The primary key of the record to delete

**Throws:**

* ``std::runtime_error``: If the record doesn't exist

**Example:**

.. code-block:: cpp

   students.remove(1001);
   std::cout << "Student deleted" << std::endl;

**Time Complexity:** O(log n) due to index update

Utility Operations
~~~~~~~~~~~~~~~~~~

contains()
^^^^^^^^^^

.. code-block:: cpp

   bool contains(const KeyType& primary_key) const;

Checks if a record with the given primary key exists.

**Parameters:**

* ``primary_key``: The primary key to check

**Returns:** ``true`` if record exists, ``false`` otherwise

**Example:**

.. code-block:: cpp

   if (students.contains(1001)) {
       std::cout << "Student 1001 exists" << std::endl;
   }

**Time Complexity:** O(log n)

size()
^^^^^^

.. code-block:: cpp

   size_t size() const;

Returns the number of records in the table.

**Returns:** Number of records

**Example:**

.. code-block:: cpp

   std::cout << "Total students: " << students.size() << std::endl;

**Time Complexity:** O(1)

Index Operations
~~~~~~~~~~~~~~~~

See :doc:`index` for comprehensive index documentation.

add_index()
^^^^^^^^^^^

.. code-block:: cpp

   template<typename FieldType>
   Table<T>& add_index(
       const Field<T, FieldType>& field,
       core::IndexType index_type
   );

Creates a secondary index on a field using fluent API.

**Parameters:**

* ``field``: Static Field object (e.g., ``Student::age``)
* ``index_type``: ``core::IndexType::Unique`` or ``core::IndexType::MultiValue``

**Returns:** Reference to ``this`` for method chaining

**Example:**

.. code-block:: cpp

   auto& students = db.table<Student>("students")
       .add_index(Student::name, core::IndexType::Unique)
       .add_index(Student::department, core::IndexType::MultiValue)
       .add_index(Student::gpa, core::IndexType::MultiValue);

find_by()
^^^^^^^^^

.. code-block:: cpp

   template<typename FieldType, typename ValueType>
   std::optional<T> find_by(
       const Field<T, FieldType>& field,
       const ValueType& value
   );

Finds a single record using a unique index.

**Parameters:**

* ``field``: Static Field object with unique index
* ``value``: Value to search for

**Returns:** ``std::optional<T>`` containing record if found

**Example:**

.. code-block:: cpp

   auto student = students.find_by(Student::name, std::string("Alice Johnson"));
   if (student) {
       std::cout << "Found: " << student->get_name() << std::endl;
   }

**Time Complexity:** O(log n) - much faster than table scan

find_all_by()
^^^^^^^^^^^^^

.. code-block:: cpp

   template<typename FieldType, typename ValueType>
   std::vector<T> find_all_by(
       const Field<T, FieldType>& field,
       const ValueType& value
   );

Finds all records matching a value using a multi-value index.

**Parameters:**

* ``field``: Static Field object with multi-value index
* ``value``: Value to search for

**Returns:** Vector of all matching records

**Example:**

.. code-block:: cpp

   auto cs_students = students.find_all_by(
       Student::department,
       std::string("CS")
   );

   std::cout << "Found " << cs_students.size() << " CS students" << std::endl;

**Time Complexity:** O(log n + k) where k = number of matches

range_query()
^^^^^^^^^^^^^

.. code-block:: cpp

   template<typename FieldType, typename ValueType>
   std::vector<T> range_query(
       const Field<T, FieldType>& field,
       const ValueType& min_value,
       const ValueType& max_value
   );

Finds all records with field values in range [min, max] (inclusive).

**Parameters:**

* ``field``: Static Field object with index
* ``min_value``: Minimum value (inclusive)
* ``max_value``: Maximum value (inclusive)

**Returns:** Vector of matching records in sorted order

**Example:**

.. code-block:: cpp

   // Find students with GPA between 3.5 and 4.0
   auto high_achievers = students.range_query(
       Student::gpa,
       3.5,
       4.0
   );

**Time Complexity:** O(log n + k) where k = number of results

get_unique_values()
^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   template<typename FieldType>
   std::vector<FieldType> get_unique_values(
       const Field<T, FieldType>& field
   );

Gets all distinct values for an indexed field.

**Parameters:**

* ``field``: Static Field object with index

**Returns:** Vector of unique values

**Example:**

.. code-block:: cpp

   auto departments = students.get_unique_values(Student::department);

   std::cout << "Departments:\n";
   for (const auto& dept : departments) {
       std::cout << "  - " << dept << std::endl;
   }

**Time Complexity:** O(n) where n = number of unique values

Complete CRUD Example
---------------------

.. code-block:: cpp

   #include <learnql/LearnQL.hpp>

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
       // 1. Create database and table
       core::Database db("university.db");
       auto& students = db.table<Student>("students");

       // 2. Insert records
       students.insert({1001, "Alice Johnson", "CS", 20, 3.8});
       students.insert({1002, "Bob Smith", "Math", 21, 3.5});

       // 3. Query records
       auto alice = students.find(1001);
       if (alice) {
           std::cout << "Found: " << alice->get_name() << std::endl;
       }

       // 4. Update a record
       if (alice) {
           alice->set_gpa(3.9);
           students.update(*alice);
       }

       // 5. Check existence
       if (students.contains(1001)) {
           std::cout << "Student 1001 exists" << std::endl;
       }

       // 6. Query with expressions
       auto high_gpa = students.where(Student::gpa >= 3.7);
       for (const auto& student : high_gpa) {
           std::cout << student.get_name() << ": " << student.get_gpa() << std::endl;
       }

       // 7. Delete a record
       students.remove(1002);

       // 8. Count records
       std::cout << "Total students: " << students.size() << std::endl;

       // Database automatically flushes on destruction
       return 0;
   }

Best Practices
--------------

1. **Keep Database Alive**

   .. code-block:: cpp

      core::Database db("data.db");
      auto& table = db.table<MyType>("mytable");
      // Both db and table are in scope

2. **Check Optional Returns**

   .. code-block:: cpp

      auto record = table.find(123);
      if (record) {  // Always check!
          process(*record);
      }

3. **Use Const When Possible**

   .. code-block:: cpp

      const auto& table = db.table<MyType>("mytable");
      // Read-only operations

4. **Handle Exceptions**

   .. code-block:: cpp

      try {
          core::Database db("data.db");
          // ...
      } catch (const std::exception& e) {
          std::cerr << "Error: " << e.what() << std::endl;
      }

5. **Use Indexes for Frequent Queries**

   .. code-block:: cpp

      auto& students = db.table<Student>("students")
          .add_index(Student::department, core::IndexType::MultiValue);

      // Future queries on department are much faster
      auto cs_students = students.find_all_by(Student::department, "CS");

See Also
--------

* :doc:`/getting-started/quick-start` - Quick introduction to core concepts
* :doc:`/tutorials/tutorial-01-first-database` - Complete tutorial using the Core module
* :doc:`query` - Query building and expressions
* :doc:`index` - Indexing and performance
* :doc:`reflection` - LEARNQL_PROPERTIES macro system
