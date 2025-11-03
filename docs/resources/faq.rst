Frequently Asked Questions
===========================

This comprehensive FAQ answers common questions about LearnQL. Questions are organized by category for easy navigation.

.. contents:: Quick Navigation
   :local:
   :depth: 2

General Questions
-----------------

What is LearnQL?
~~~~~~~~~~~~~~~~

LearnQL is a modern, type-safe C++20 database library designed for **learning and prototyping**. It demonstrates advanced C++20 features through a practical database implementation, making it ideal for:

- Learning modern C++ features (concepts, coroutines, ranges)
- Understanding database internals
- Rapid prototyping of applications
- Educational projects and experiments

LearnQL features:

- SQL-like query DSL with expression templates
- Compile-time type safety and reflection
- Persistent page-based storage
- B-tree indexing for fast lookups
- Property macros that reduce boilerplate
- Full C++20 ranges and coroutines support

**Important**: LearnQL is NOT production-ready. It's a teaching tool and prototype framework.

**See Also**: :doc:`../getting-started/core-concepts`, :doc:`../index`

Is LearnQL production-ready?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**No.** LearnQL is explicitly designed for **learning and prototyping**, not production use.

**Limitations**:

- No concurrent access (single-threaded only)
- No transaction support or ACID guarantees
- No query optimizer
- Limited to in-process usage (no network protocol)
- Not tested for large-scale datasets (> 1M records)
- Basic error handling and recovery
- No write-ahead logging (WAL)
- No MVCC for concurrent reads

**For Production Use**: Consider established databases like:

- SQLite - Embedded relational database
- RocksDB - High-performance key-value store
- PostgreSQL - Full-featured RDBMS
- MongoDB - Document database

**When to Use LearnQL**:

- Learning C++20 features
- Understanding database internals
- Rapid prototyping
- Educational projects
- Code examples and demos

**See Also**: :doc:`../getting-started/best-practices`, :ref:`How does LearnQL compare to SQLite?`

Why learn LearnQL?
~~~~~~~~~~~~~~~~~~

LearnQL offers unique educational value:

**1. Learn Modern C++20**

See real-world applications of:

- Concepts and constraints for type safety
- Coroutines for lazy evaluation
- Ranges for composable operations
- Expression templates for DSL design
- Compile-time reflection
- Template metaprogramming

**2. Understand Database Internals**

Learn how databases work:

- Page-based storage engines
- B-tree indexing algorithms
- Query execution and optimization
- Serialization and persistence
- Schema management

**3. Practical Application**

Not just theory - build real applications:

- Type-safe query DSL
- Persistent storage
- Secondary indexes
- System catalog
- Debugging tools

**4. Well-Documented**

Every feature is explained:

- Comprehensive tutorials
- Architecture documentation
- Inline code documentation
- Real-world examples

**See Also**: :doc:`../tutorials/tutorial-01-first-database`, :doc:`examples`

Who should use LearnQL?
~~~~~~~~~~~~~~~~~~~~~~~

**Perfect For**:

- **C++ Learners**: Want to see modern C++20 in practice
- **Students**: Learning database concepts or C++
- **Educators**: Teaching database internals or C++20
- **Prototypers**: Need quick data persistence for demos
- **Library Developers**: Learning API design patterns

**Not Suitable For**:

- Production applications
- Large-scale data processing
- Multi-user systems
- Performance-critical applications
- Systems requiring ACID guarantees

**See Also**: :doc:`../getting-started/quick-start`

What license is LearnQL under?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

LearnQL is licensed under the **Academic Free License 3.0** (AFL-3.0).

**Key Points**:

- Open source and free to use
- Can be used in academic and commercial projects
- Must retain copyright notices
- No warranty provided

See the ``LICENSE`` file in the repository for full details.

Getting Started
---------------

What C++20 features do I need to know?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To use LearnQL effectively, you should understand:

**Essential Features**:

- **auto** - Type deduction
- **Lambdas** - Anonymous functions
- **Structured bindings** - Unpacking tuples/structs
- **Range-based for loops** - Iteration
- **Move semantics** - Efficient resource transfer

**Helpful Features**:

- **Concepts** - Type constraints
- **Ranges** - Composable operations
- **Coroutines** - Lazy evaluation
- **constexpr** - Compile-time evaluation

**Advanced Features** (optional):

- Expression templates
- Template metaprogramming
- CRTP pattern
- Perfect forwarding

**Learning Resources**:

- :doc:`cpp20-glossary` - Comprehensive glossary of C++20 terms
- :doc:`../getting-started/core-concepts` - LearnQL concepts
- `learncpp.com <https://www.learncpp.com/>`_ - Free C++ tutorial
- `cppreference.com <https://en.cppreference.com/>`_ - C++ reference

**See Also**: :doc:`cpp20-glossary`

How do I enable C++20 in my compiler?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**GCC (10+)**:

.. code-block:: bash

   g++ -std=c++20 main.cpp -o main

**Clang (12+)**:

.. code-block:: bash

   clang++ -std=c++20 main.cpp -o main

**MSVC (19.29+)**:

.. code-block:: bash

   cl /std:c++20 main.cpp

**CMake**:

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.23)
   project(MyProject)

   set(CMAKE_CXX_STANDARD 20)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)

   add_executable(main main.cpp)
   target_link_libraries(main PRIVATE learnql)

**Verify C++20 Support**:

.. code-block:: cpp

   #include <iostream>

   int main() {
   #if __cplusplus >= 202002L
       std::cout << "C++20 supported!\n";
   #else
       std::cout << "C++20 NOT supported\n";
   #endif
   }

**See Also**: :doc:`../getting-started/installation`

What are the minimum requirements?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Compiler Support**:

- GCC 10 or higher
- Clang 12 or higher
- MSVC 19.29 (Visual Studio 2019 16.11) or higher

**Build Tools**:

- CMake 3.23 or higher (recommended)
- C++20 standard library

**Operating Systems**:

- Linux (tested on Ubuntu 20.04+)
- macOS (tested on 11+)
- Windows (tested on Windows 10+)

**Hardware**:

- Any modern CPU (x86_64, ARM64)
- Minimal memory requirements

**No External Dependencies**: LearnQL is header-only with zero dependencies.

**See Also**: :doc:`../getting-started/installation`

How do I install LearnQL?
~~~~~~~~~~~~~~~~~~~~~~~~~~

LearnQL is a header-only library - just include it!

**Option 1: CMake (Recommended)**

.. code-block:: cmake

   # Add LearnQL to your project
   add_subdirectory(path/to/LearnQL)
   target_link_libraries(your_target PRIVATE learnql)

**Option 2: Direct Include**

Copy the ``learnql/`` directory to your include path:

.. code-block:: bash

   cp -r LearnQL/learnql /usr/local/include/

**Option 3: Git Submodule**

.. code-block:: bash

   git submodule add https://github.com/tdelphi1981/LearnQL.git external/LearnQL
   git submodule update --init --recursive

Then in CMakeLists.txt:

.. code-block:: cmake

   add_subdirectory(external/LearnQL)

**Verify Installation**:

.. code-block:: cpp

   #include <learnql/LearnQL.hpp>
   #include <iostream>

   int main() {
       std::cout << "LearnQL installed successfully!\n";
   }

**See Also**: :doc:`../getting-started/installation`, :doc:`../getting-started/quick-start`

Features and Usage
------------------

What is a property macro?
~~~~~~~~~~~~~~~~~~~~~~~~~~

Property macros are LearnQL's code generation system that transforms simple field declarations into full database-aware properties.

**Without LearnQL**:

.. code-block:: cpp

   class Student {
       int id_;
       std::string name_;
   public:
       int id() const { return id_; }
       void set_id(int v) { id_ = v; }
       std::string name() const { return name_; }
       void set_name(const std::string& v) { name_ = v; }

       // Manual serialization code
       std::vector<std::byte> serialize() const { /* ... */ }
       void deserialize(const std::vector<std::byte>& data) { /* ... */ }

       // Static field objects for queries
       static Field<int> id_field;
       static Field<std::string> name_field;
   };

**With LearnQL Property Macros**:

.. code-block:: cpp

   class Student {
       LEARNQL_PROPERTIES_BEGIN(Student)
           LEARNQL_PROPERTY(int, id, PK)
           LEARNQL_PROPERTY(std::string, name)
       LEARNQL_PROPERTIES_END(
           PROP(int, id, PK),
           PROP(std::string, name)
       )
   };

   // ✓ Getters: id(), name()
   // ✓ Setters: set_id(), set_name()
   // ✓ Static fields: Student::id, Student::name
   // ✓ Serialization: serialize(), deserialize()
   // ✓ Reflection metadata

**Reduces boilerplate by ~70%!**

**See Also**: :doc:`../getting-started/property-macros`, :doc:`../tutorials/tutorial-01-first-database`

How do I create relationships between tables?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

LearnQL supports relationships through foreign keys and joins.

**One-to-Many Example** (Student → Enrollments):

.. code-block:: cpp

   class Student {
       LEARNQL_PROPERTIES_BEGIN(Student)
           LEARNQL_PROPERTY(int, student_id, PK)
           LEARNQL_PROPERTY(std::string, name)
       LEARNQL_PROPERTIES_END(
           PROP(int, student_id, PK),
           PROP(std::string, name)
       )
   };

   class Enrollment {
       LEARNQL_PROPERTIES_BEGIN(Enrollment)
           LEARNQL_PROPERTY(int, enrollment_id, PK)
           LEARNQL_PROPERTY(int, student_id)  // Foreign key
           LEARNQL_PROPERTY(std::string, course)
       LEARNQL_PROPERTIES_END(
           PROP(int, enrollment_id, PK),
           PROP(int, student_id),
           PROP(std::string, course)
       )
   };

   // Query with join
   auto results = students.query()
       .innerJoin(enrollments,
           Student::student_id == Enrollment::student_id)
       .collect();

**Many-to-Many Example** (Students ↔ Courses):

.. code-block:: cpp

   class StudentCourse {  // Junction table
       LEARNQL_PROPERTIES_BEGIN(StudentCourse)
           LEARNQL_PROPERTY(int, id, PK)
           LEARNQL_PROPERTY(int, student_id)
           LEARNQL_PROPERTY(int, course_id)
       LEARNQL_PROPERTIES_END(
           PROP(int, id, PK),
           PROP(int, student_id),
           PROP(int, course_id)
       )
   };

   // Find all courses for a student
   auto student_courses = students.query()
       .innerJoin(student_courses, Student::student_id == StudentCourse::student_id)
       .innerJoin(courses, StudentCourse::course_id == Course::course_id)
       .where(Student::student_id == 1)
       .collect();

**See Also**: :doc:`../tutorials/tutorial-04-joins-relationships`, :doc:`examples`

How do I create indexes?
~~~~~~~~~~~~~~~~~~~~~~~~~

Indexes speed up queries on non-primary-key fields.

**Creating an Index**:

.. code-block:: cpp

   // Create index on department field
   students.createIndex<&Student::department>("idx_department");

**Index Types**:

- **Primary Key Index**: Automatically created for the first property
- **Secondary Indexes**: B-tree indexes you create manually

**When to Use Indexes**:

- Frequent lookups on a field
- WHERE clauses on specific fields
- JOIN operations
- ORDER BY operations

**Index Performance**:

.. code-block:: cpp

   // Without index: O(n) - scans all records
   auto cs_students = students.query()
       .where(Student::department == "CS")
       .collect();

   // With index: O(log n) - uses B-tree lookup
   students.createIndex<&Student::department>("idx_dept");
   auto cs_students = students.query()
       .where(Student::department == "CS")  // Uses index!
       .collect();

**Index Tradeoffs**:

- **Pros**: Faster queries
- **Cons**: Slower inserts/updates, more disk space

**See Also**: :doc:`../tutorials/tutorial-06-indexes-performance`, :doc:`../api/index`

Can I use LearnQL with threads?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**No, LearnQL is single-threaded only.**

**Why No Threading?**:

- Simplifies implementation for educational purposes
- No locking overhead or complexity
- Easier to understand database internals

**Workarounds**:

**1. Separate Databases per Thread**:

.. code-block:: cpp

   // Each thread has its own database file
   void workerThread(int thread_id) {
       Database db("database_" + std::to_string(thread_id) + ".db");
       auto& table = db.table<Record>("records");
       // Use table...
   }

**2. Single-Writer Pattern**:

.. code-block:: cpp

   std::mutex db_mutex;
   Database db("shared.db");

   void safeWrite(const Record& record) {
       std::lock_guard<std::mutex> lock(db_mutex);
       db.table<Record>("records").insert(record);
   }

**Warning**: This defeats performance gains from threading!

**For Production**: Use a real database with proper concurrency support (PostgreSQL, SQLite with WAL mode, etc.).

**See Also**: :ref:`Is LearnQL production-ready?`

How do I perform aggregations?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

LearnQL supports SQL-like aggregations with GROUP BY.

**Count Records**:

.. code-block:: cpp

   auto count = students.query().collect().size();

**Sum/Average/Min/Max**:

.. code-block:: cpp

   auto students_vec = students.query().collect();

   // Sum
   double total_gpa = std::accumulate(students_vec.begin(), students_vec.end(), 0.0,
       [](double sum, const auto& s) { return sum + s.gpa(); });

   // Average
   double avg_gpa = total_gpa / students_vec.size();

   // Max
   auto max_student = std::max_element(students_vec.begin(), students_vec.end(),
       [](const auto& a, const auto& b) { return a.gpa() < b.gpa(); });

**GROUP BY**:

.. code-block:: cpp

   auto dept_stats = students.query()
       .groupBy<&Student::department>()
       .aggregate([](const auto& group) {
           double avg = std::accumulate(group.begin(), group.end(), 0.0,
               [](double sum, const auto& s) { return sum + s.gpa(); }) / group.size();

           return std::make_pair(group.key(), avg);
       })
       .collect();

   // Print results
   for (const auto& [dept, avg_gpa] : dept_stats) {
       std::cout << dept << ": " << avg_gpa << '\n';
   }

**See Also**: :doc:`../tutorials/tutorial-05-aggregations-groupby`, :doc:`examples`

Troubleshooting
---------------

Why is my debug build so slow?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Debug builds are intentionally slow due to:

**1. No Optimizations**

Debug mode disables compiler optimizations for easier debugging.

**2. Debug Symbols**

Extensive debugging information is included.

**3. Assertions and Checks**

Extra runtime validation.

**Solution**: Use release builds for performance testing.

**CMake Configuration**:

.. code-block:: bash

   # Debug build (slow)
   cmake -DCMAKE_BUILD_TYPE=Debug ..

   # Release build (fast)
   cmake -DCMAKE_BUILD_TYPE=Release ..

**Performance Comparison**:

.. code-block:: text

   Debug:    10,000 inserts in ~5000ms
   Release:  10,000 inserts in ~50ms (100x faster!)

**Best Practice**: Develop in debug, test performance in release.

**See Also**: :doc:`../architecture/performance`

Why am I getting concept errors?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Concept errors mean your type doesn't satisfy required constraints.

**Common Error**:

.. code-block:: text

   error: constraints not satisfied
   note: the required expression 'T::serialize()' is invalid

**Cause**: Your class doesn't have required methods.

**Solution**: Ensure your class has all required properties.

**Example Fix**:

.. code-block:: cpp

   // Wrong - missing property macros
   struct Student {
       int id;
       std::string name;
   };

   // Correct - using property macros
   struct Student {
       LEARNQL_PROPERTIES_BEGIN(Student)
           LEARNQL_PROPERTY(int, id, PK)
           LEARNQL_PROPERTY(std::string, name)
       LEARNQL_PROPERTIES_END(
           PROP(int, id, PK),
           PROP(std::string, name)
       )
   };

**Reading Concept Errors**:

1. Look for "constraints not satisfied"
2. Find the missing requirement
3. Add required method/property
4. Recompile

**See Also**: :doc:`cpp20-glossary` (Concepts section), :doc:`../getting-started/property-macros`

Why doesn't my query compile?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Common Issues**:

**1. Wrong Field Type**:

.. code-block:: cpp

   // Wrong - comparing string field to int
   students.query().where(Student::name == 42);  // ERROR!

   // Correct - use matching types
   students.query().where(Student::name == "Alice");

**2. Missing Field Object**:

.. code-block:: cpp

   // Wrong - using member variable
   students.query().where(student.age > 18);  // ERROR!

   // Correct - use static field object
   students.query().where(Student::age > 18);

**3. Incorrect Syntax**:

.. code-block:: cpp

   // Wrong - C++ doesn't have BETWEEN operator
   students.query().where(Student::age BETWEEN 18 AND 25);  // ERROR!

   // Correct - use logical operators
   students.query().where(Student::age >= 18 && Student::age <= 25);

**See Also**: :doc:`../tutorials/tutorial-03-query-dsl`, :doc:`../api/query`

How do I debug query issues?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

LearnQL provides debugging tools:

**1. Print Query Plan**:

.. code-block:: cpp

   auto query = students.query()
       .where(Student::gpa > 3.5);

   // Visual inspection (if implemented)
   std::cout << query.toString() << '\n';

**2. Use Profiler**:

.. code-block:: cpp

   #include <learnql/debug/Profiler.hpp>

   debug::Profiler profiler;
   profiler.start("my_query");

   auto results = students.query()
       .where(Student::department == "CS")
       .collect();

   profiler.stop("my_query");
   std::cout << profiler.report() << '\n';

**3. Database Inspector**:

.. code-block:: cpp

   #include <learnql/utils/DbInspector.hpp>

   utils::DbInspector inspector(db);
   inspector.printDatabaseStructure();
   inspector.printTableStatistics("students");

**4. Manual Iteration**:

.. code-block:: cpp

   // Step through results
   for (const auto& student : students.scan()) {
       if (student.gpa() > 3.5) {
           std::cout << student.name() << '\n';
       }
   }

**See Also**: :doc:`../api/debug`, :doc:`../tutorials/tutorial-07-advanced-features`

Comparison with Other Databases
--------------------------------

How does LearnQL compare to SQLite?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - Feature
     - LearnQL
     - SQLite
   * - **Purpose**
     - Learning & prototyping
     - Production use
   * - **Type Safety**
     - Compile-time
     - Runtime
   * - **Query Language**
     - C++ expression templates
     - SQL strings
   * - **Schema**
     - C++ structs
     - SQL DDL
   * - **Thread Safety**
     - No
     - Yes (with proper mode)
   * - **Transactions**
     - No
     - Yes (ACID)
   * - **Concurrency**
     - Single-threaded
     - Multi-reader, single-writer
   * - **Performance**
     - Moderate
     - Excellent
   * - **Maturity**
     - Educational
     - Production-proven
   * - **Dependencies**
     - None
     - None
   * - **Size**
     - Header-only
     - ~600KB library
   * - **Learning Curve**
     - C++20 required
     - SQL knowledge required

**When to Use LearnQL**:

- Learning C++20 and database internals
- Type-safe queries at compile-time
- Rapid prototyping in C++
- Educational projects

**When to Use SQLite**:

- Production applications
- Multi-threaded access
- ACID transactions needed
- Cross-language access
- Mature, stable platform

**See Also**: :ref:`Is LearnQL production-ready?`

Can LearnQL replace my SQL database?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Short Answer**: No, not for production.

**Why Not**:

- No SQL interface
- No network protocol
- No concurrent access
- No transactions
- No query optimizer
- Limited scalability

**What LearnQL IS Good For**:

1. **Learning**: Understanding database concepts
2. **Prototyping**: Quick data persistence in C++
3. **Embedded Use**: Simple single-threaded applications
4. **Examples**: Demo applications and code samples

**Migration Path**:

If your prototype grows:

.. code-block:: cpp

   // Start with LearnQL for prototyping
   Database db("prototype.db");
   auto& users = db.table<User>("users");
   users.insert(User{1, "Alice"});

   // Later migrate to SQLite, PostgreSQL, etc.
   // Convert LearnQL queries to SQL
   // Use proper database for production

**See Also**: :doc:`examples`, :ref:`Is LearnQL production-ready?`

Performance
-----------

How do I optimize query performance?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**1. Use Indexes**:

.. code-block:: cpp

   // Slow: O(n) full table scan
   auto results = students.query()
       .where(Student::department == "CS")
       .collect();

   // Fast: O(log n) index lookup
   students.createIndex<&Student::department>("idx_dept");
   auto results = students.query()
       .where(Student::department == "CS")
       .collect();

**2. Avoid Unnecessary Collections**:

.. code-block:: cpp

   // Bad: Collects all results first
   auto all = students.query().collect();
   for (const auto& s : all | std::views::take(10)) { }

   // Better: Use coroutines for streaming
   auto gen = students.queryGenerator()
       .where(Student::gpa > 3.5);
   for (int i = 0; i < 10; ++i) {
       auto student = gen.next();
   }

**3. Batch Operations**:

.. code-block:: cpp

   // Slow: Individual inserts
   for (const auto& student : students_to_add) {
       students.insert(student);
   }

   // Faster: Batch loading (if available)
   students.insertBatch(students_to_add);

**4. Use Release Builds**:

.. code-block:: bash

   cmake -DCMAKE_BUILD_TYPE=Release ..

**5. Profile Your Code**:

.. code-block:: cpp

   debug::Profiler profiler;
   profiler.start("query");
   auto results = /* your query */;
   profiler.stop("query");
   std::cout << profiler.report();

**See Also**: :doc:`../tutorials/tutorial-06-indexes-performance`, :doc:`../architecture/performance`

What's the maximum database size?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Theoretical Limit**: Limited by:

- Disk space
- File system limits
- RecordId addressing (typically 32 or 64-bit)

**Practical Limits**:

LearnQL is designed for **small to medium datasets**:

- **Recommended**: < 100,000 records
- **Maximum tested**: ~1,000,000 records
- **Performance degradation**: Beyond 1M records

**Why Limited?**:

- No query optimizer
- Simple B-tree implementation
- Single-threaded
- Not optimized for large datasets

**For Large Datasets**: Use production databases (PostgreSQL, MongoDB, etc.).

**See Also**: :ref:`Is LearnQL production-ready?`, :doc:`../architecture/performance`

Advanced Topics
---------------

How do I back up my database?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Simple File Copy**:

.. code-block:: cpp

   // Close database first
   db.~Database();

   // Copy database file
   std::filesystem::copy_file("database.db", "database_backup.db");

**Programmatic Backup**:

.. code-block:: cpp

   void backupDatabase(const std::string& source, const std::string& dest) {
       // Ensure database is closed or read-only
       std::ifstream src(source, std::ios::binary);
       std::ofstream dst(dest, std::ios::binary);
       dst << src.rdbuf();
   }

**Best Practices**:

1. Close database before copying
2. Use atomic file operations
3. Verify backup integrity
4. Regular backup schedule

**See Also**: :doc:`../api/storage`

Can I query the database from multiple processes?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**No, multi-process access is not supported.**

**Why Not**:

- No file locking mechanism
- No inter-process synchronization
- Risk of data corruption

**Workarounds**:

**1. Single-Writer Process**:

.. code-block:: cpp

   // Process 1: Writer
   Database db("data.db");
   db.table<Record>("records").insert(record);

   // Process 2: Reader (copy database first)
   std::filesystem::copy("data.db", "data_copy.db");
   Database db_readonly("data_copy.db");
   auto results = db_readonly.table<Record>("records").scan();

**2. Separate Databases**:

Each process uses its own database file.

**For Multi-Process**: Use client-server database (PostgreSQL, MySQL, etc.).

**See Also**: :ref:`Can I use LearnQL with threads?`

How do I migrate data between versions?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

LearnQL doesn't have automatic migrations. Manual migration required:

**Migration Process**:

.. code-block:: cpp

   // Old schema
   struct UserV1 {
       LEARNQL_PROPERTIES_BEGIN(UserV1)
           LEARNQL_PROPERTY(int, id, PK)
           LEARNQL_PROPERTY(std::string, name)
       LEARNQL_PROPERTIES_END(
           PROP(int, id, PK),
           PROP(std::string, name)
       )
   };

   // New schema (added email)
   struct UserV2 {
       LEARNQL_PROPERTIES_BEGIN(UserV2)
           LEARNQL_PROPERTY(int, id, PK)
           LEARNQL_PROPERTY(std::string, name)
           LEARNQL_PROPERTY(std::string, email)
       LEARNQL_PROPERTIES_END(
           PROP(int, id, PK),
           PROP(std::string, name),
           PROP(std::string, email)
       )
   };

   // Migration function
   void migrate_v1_to_v2() {
       Database old_db("users_v1.db");
       Database new_db("users_v2.db");

       auto& old_users = old_db.table<UserV1>("users");
       auto& new_users = new_db.table<UserV2>("users");

       for (const auto& old_user : old_users.scan()) {
           UserV2 new_user;
           new_user.set_id(old_user.id());
           new_user.set_name(old_user.name());
           new_user.set_email("");  // Default value
           new_users.insert(new_user);
       }
   }

**Best Practices**:

1. Version your schema
2. Keep old code for reading old databases
3. Write migration scripts
4. Test migrations on copies
5. Backup before migrating

Can I extend LearnQL with custom types?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Yes**, but they must be serializable.

**Built-in Supported Types**:

- Integers: ``int``, ``long``, ``int64_t``, etc.
- Floating-point: ``float``, ``double``
- Strings: ``std::string``
- Boolean: ``bool``

**Adding Custom Type**:

.. code-block:: cpp

   struct Date {
       int year, month, day;

       // Required: Serialization
       std::vector<std::byte> serialize() const {
           std::vector<std::byte> data;
           // Serialize year, month, day...
           return data;
       }

       static Date deserialize(const std::vector<std::byte>& data) {
           Date date;
           // Deserialize data...
           return date;
       }

       // Optional: Comparison operators for queries
       bool operator==(const Date& other) const {
           return year == other.year && month == other.month && day == other.day;
       }

       bool operator<(const Date& other) const {
           // Implement comparison...
       }
   };

   // Use in property
   struct Event {
       LEARNQL_PROPERTIES_BEGIN(Event)
           LEARNQL_PROPERTY(int, id, PK)
           LEARNQL_PROPERTY(Date, event_date)
       LEARNQL_PROPERTIES_END(
           PROP(int, id, PK),
           PROP(Date, event_date)
       )
   };

**Requirements**:

- ``serialize()`` method
- ``deserialize()`` static method
- Comparison operators (for queries)

**See Also**: :doc:`../api/serialization`

Contributing and Community
--------------------------

How can I contribute?
~~~~~~~~~~~~~~~~~~~~~

Contributions are welcome! See :doc:`contributing` for full guidelines.

**Ways to Contribute**:

1. **Report Bugs**: Open issues on GitHub
2. **Fix Bugs**: Submit pull requests
3. **Add Features**: Implement new functionality
4. **Improve Docs**: Enhance documentation
5. **Write Examples**: Create tutorials and examples
6. **Answer Questions**: Help other users

**Good First Issues**:

- Documentation improvements
- Additional examples
- Unit tests
- Performance benchmarks
- Bug fixes in non-critical paths

**See Also**: :doc:`contributing`

Where can I get help?
~~~~~~~~~~~~~~~~~~~~~

**Documentation**:

- :doc:`../getting-started/quick-start` - Getting started guide
- :doc:`../tutorials/tutorial-01-first-database` - First tutorial
- :doc:`cpp20-glossary` - C++20 terminology
- :doc:`examples` - Working code examples

**GitHub**:

- Open an issue with the "question" label
- Check existing issues and discussions
- Review closed issues for similar problems

**Community**:

- Follow the project on GitHub
- Star the repository to show support
- Share your projects using LearnQL

**See Also**: :doc:`contributing`

Is there a community or forum?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Currently, LearnQL uses **GitHub Issues** for community interaction:

- Questions: Open issue with "question" label
- Bugs: Open issue with "bug" label
- Feature requests: Open issue with "enhancement" label
- Discussions: Use GitHub Discussions (if enabled)

**Best Practices**:

1. Search existing issues first
2. Provide minimal reproducible examples
3. Include environment details (OS, compiler, versions)
4. Be respectful and constructive

**See Also**: :doc:`contributing`

Quick Reference
---------------

Common Error Messages
~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 50 50

   * - Error
     - Solution
   * - "constraints not satisfied"
     - Add property macros to your class
   * - "no matching function for call"
     - Check field types match in queries
   * - "incomplete type"
     - Include missing header files
   * - "cannot open database file"
     - Check file path and permissions
   * - Slow debug performance
     - Use release build for performance testing

Essential Links
~~~~~~~~~~~~~~~

- :doc:`../getting-started/installation` - Installation guide
- :doc:`../getting-started/quick-start` - Quick start tutorial
- :doc:`../tutorials/tutorial-01-first-database` - First database tutorial
- :doc:`cpp20-glossary` - C++20 glossary
- :doc:`examples` - Complete code examples
- :doc:`contributing` - Contributing guide

Next Steps
----------

**New Users**:

1. :doc:`../getting-started/installation` - Install LearnQL
2. :doc:`../getting-started/quick-start` - Run your first example
3. :doc:`../tutorials/tutorial-01-first-database` - Build a database
4. :doc:`examples` - Study real examples

**Learning C++20**:

1. :doc:`cpp20-glossary` - Learn C++20 terms
2. :doc:`../architecture/expression-templates` - Study design patterns
3. :doc:`../architecture/reflection-system` - Understand metaprogramming

**Contributing**:

1. :doc:`contributing` - Read contribution guidelines
2. Find "good first issue" on GitHub
3. Join the community

Need More Help?
---------------

Can't find your answer? Try:

1. **Search the docs**: Use the search box above
2. **Check examples**: :doc:`examples` has working code
3. **Read tutorials**: :doc:`../tutorials/tutorial-01-first-database`
4. **Ask on GitHub**: Open an issue with "question" label

**See Also**: :doc:`contributing`, :doc:`examples`

----

**Last Updated**: 2025-11-02

**Found an error or have a suggestion?** Please open an issue on GitHub or see :doc:`contributing`.
