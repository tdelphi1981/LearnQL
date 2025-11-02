Best Practices
==============

This guide covers best practices for using LearnQL effectively, with a focus on helping C++ beginners write clean, efficient, and maintainable database code.

Database Design
---------------

Choose Good Primary Keys
~~~~~~~~~~~~~~~~~~~~~~~~~

✅ **Do: Use integer primary keys**

.. code-block:: cpp

   class Student {
       LEARNQL_PROPERTIES_BEGIN(Student)
           LEARNQL_PROPERTY(int, student_id, PK)  // Fast, compact, auto-indexed
           LEARNQL_PROPERTY(std::string, name)
       LEARNQL_PROPERTIES_END(
           PROP(int, student_id, PK),
           PROP(std::string, name)
       )
   };

❌ **Avoid: Large string primary keys**

.. code-block:: cpp

   class Student {
       LEARNQL_PROPERTIES_BEGIN(Student)
           LEARNQL_PROPERTY(std::string, student_email, PK)  // Slower lookups
           LEARNQL_PROPERTY(std::string, name)
       LEARNQL_PROPERTIES_END(
           PROP(std::string, student_email, PK),
           PROP(std::string, name)
       )
   };

**Why?** Integer keys are:

* Faster to compare
* More compact in memory
* More efficient for B-tree indexing

Normalize Your Data
~~~~~~~~~~~~~~~~~~~

✅ **Do: Use foreign keys to avoid duplication**

.. code-block:: cpp

   class Department {
       LEARNQL_PROPERTIES_BEGIN(Department)
           LEARNQL_PROPERTY(int, dept_id, PK)
           LEARNQL_PROPERTY(std::string, name)
       LEARNQL_PROPERTIES_END(
           PROP(int, dept_id, PK),
           PROP(std::string, name)
       )
   };

   class Employee {
       LEARNQL_PROPERTIES_BEGIN(Employee)
           LEARNQL_PROPERTY(int, employee_id, PK)
           LEARNQL_PROPERTY(std::string, name)
           LEARNQL_PROPERTY(int, department_id)  // Foreign key
       LEARNQL_PROPERTIES_END(
           PROP(int, employee_id, PK),
           PROP(std::string, name),
           PROP(int, department_id)
       )
   };

❌ **Avoid: Duplicating data**

.. code-block:: cpp

   class Employee {
       LEARNQL_PROPERTIES_BEGIN(Employee)
           LEARNQL_PROPERTY(int, employee_id, PK)
           LEARNQL_PROPERTY(std::string, name)
           LEARNQL_PROPERTY(std::string, department_name)  // Duplicated!
       LEARNQL_PROPERTIES_END(
           PROP(int, employee_id, PK),
           PROP(std::string, name),
           PROP(std::string, department_name)
       )
   };

Query Optimization
------------------

Use Indexes for Frequent Queries
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

✅ **Do: Create secondary indexes for frequently queried fields**

.. code-block:: cpp

   auto& students = db.table<Student>("students")
       .add_index(Student::department, core::IndexType::MultiValue);

   // Now this query is much faster (O(log n) instead of O(n))
   auto cs_students = students.find_all_by(Student::department, std::string("CS"));

❌ **Avoid: Full table scans for repeated queries**

.. code-block:: cpp

   // Slow if repeated many times (scans entire table each time)
   auto cs_students = students.where(Student::department == "CS");

Avoid Repeated Queries in Loops
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

❌ **Avoid: N+1 query problem**

.. code-block:: cpp

   // Bad: One query per student
   for (const auto& student : students) {
       auto dept = departments.find(student.get_department_id());
       if (dept) {
           std::cout << student.get_name() << " - " << dept->get_name() << std::endl;
       }
   }

✅ **Do: Batch queries or preload data**

.. code-block:: cpp

   // Good: Load all departments once
   std::unordered_map<int, Department> dept_map;
   for (const auto& dept : departments) {
       dept_map[dept.get_dept_id()] = dept;
   }

   // Now just one table scan
   for (const auto& student : students) {
       const auto& dept = dept_map[student.get_department_id()];
       std::cout << student.get_name() << " - " << dept.get_name() << std::endl;
   }

Error Handling
--------------

Always Check Optional Returns
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

✅ **Do: Check if record exists before accessing**

.. code-block:: cpp

   auto student = students.find(123);

   if (student) {
       std::cout << "Found: " << student->get_name() << std::endl;
   } else {
       std::cout << "Student not found" << std::endl;
   }

❌ **Avoid: Dereferencing without checking**

.. code-block:: cpp

   auto student = students.find(123);
   std::cout << student->get_name();  // CRASH if not found!

Handle Errors Gracefully
~~~~~~~~~~~~~~~~~~~~~~~~~

✅ **Do: Use try-catch for I/O operations**

.. code-block:: cpp

   try {
       core::Database db("university.db");
       auto& students = db.table<Student>("students");
       // ... operations ...
   } catch (const std::exception& e) {
       std::cerr << "Database error: " << e.what() << std::endl;
       return 1;
   }

Code Organization
-----------------

Separate Data Models
~~~~~~~~~~~~~~~~~~~~

✅ **Do: Define models in separate headers**

.. code-block:: cpp

   // models/student.hpp
   #ifndef MODELS_STUDENT_HPP
   #define MODELS_STUDENT_HPP

   #include <learnql/LearnQL.hpp>

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

   #endif

   // main.cpp
   #include "models/student.hpp"

Use Meaningful Table Names
~~~~~~~~~~~~~~~~~~~~~~~~~~~

✅ **Do: Use clear, descriptive table names**

.. code-block:: cpp

   auto& students = db.table<Student>("students");
   auto& courses = db.table<Course>("courses");
   auto& enrollments = db.table<Enrollment>("enrollments");

❌ **Avoid: Cryptic or inconsistent names**

.. code-block:: cpp

   auto& s = db.table<Student>("s");
   auto& course_tbl = db.table<Course>("crse");

Encapsulate Database Logic
~~~~~~~~~~~~~~~~~~~~~~~~~~~

✅ **Do: Create repository classes**

.. code-block:: cpp

   class StudentRepository {
   private:
       learnql::core::Database& db;
       learnql::core::Table<Student>& students;

   public:
       StudentRepository(learnql::core::Database& db)
           : db(db), students(db.table<Student>("students")) {}

       std::optional<Student> find_by_id(int id) {
           return students.find(id);
       }

       std::vector<Student> find_by_department(const std::string& dept) {
           std::vector<Student> result;
           for (const auto& s : students.where(Student::department == dept)) {
               result.push_back(s);
           }
           return result;
       }

       void save(const Student& student) {
           students.insert(student);
       }

       void update_student(const Student& student) {
           students.update(student);
       }
   };

   // Usage
   StudentRepository repo(db);
   auto student = repo.find_by_id(123);

❌ **Avoid: Scattering database code throughout your application**

Performance Tips
----------------

Use In-Memory Databases for Testing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

✅ **Do: Use ``:memory:`` for fast unit tests**

.. code-block:: cpp

   TEST(StudentTest, InsertAndQuery) {
       core::Database db(":memory:");  // Fast, no disk I/O
       auto& students = db.table<Student>("students");

       students.insert(Student(1, "Alice", 20));

       auto result = students.find(1);
       ASSERT_TRUE(result.has_value());
   }

Compile in Release Mode
~~~~~~~~~~~~~~~~~~~~~~~~

LearnQL uses heavy template metaprogramming that benefits from optimization:

.. code-block:: bash

   # Debug mode (slow, but good for development)
   cmake -DCMAKE_BUILD_TYPE=Debug ..

   # Release mode (fast, optimized)
   cmake -DCMAKE_BUILD_TYPE=Release ..

**Performance difference can be 10-100x!**

Batch Operations When Possible
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

✅ **Do: Insert multiple records efficiently**

.. code-block:: cpp

   // If you need to insert many records, do it in a loop
   std::vector<Student> students_to_insert = load_students_from_csv();
   for (const auto& student : students_to_insert) {
       students.insert(student);
   }

Memory Management
-----------------

Be Careful with Large Result Sets
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

✅ **Do: Use iterators for large datasets**

.. code-block:: cpp

   // Good: Iterates without loading everything into memory
   for (const auto& student : students.where(Student::age >= 21)) {
       process(student);
   }

❌ **Avoid: Copying large result sets**

.. code-block:: cpp

   // Bad: Copies all results into a vector
   std::vector<Student> vec;
   for (const auto& s : students) {
       vec.push_back(s);
   }

Close Databases Properly
~~~~~~~~~~~~~~~~~~~~~~~~~

✅ **Do: Let RAII handle cleanup**

.. code-block:: cpp

   void process_data() {
       core::Database db("data.db");
       // ... use database ...
   }  // ← Automatically flushed and closed

❌ **Avoid: Keeping databases open unnecessarily**

.. code-block:: cpp

   // Avoid global database objects
   core::Database global_db("data.db");  // Stays open entire program

Property Access Best Practices
-------------------------------

Always Use Getters and Setters
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

✅ **Do: Use generated getter/setter methods**

.. code-block:: cpp

   Student student;

   // Use setters
   student.set_student_id(1001);
   student.set_name("Alice Johnson");
   student.set_age(20);

   // Use getters
   std::cout << "Name: " << student.get_name() << std::endl;
   std::cout << "Age: " << student.get_age() << std::endl;

❌ **Avoid: Accessing private members directly**

.. code-block:: cpp

   Student student;
   student.student_id_ = 1001;  // Won't compile - private member!
   student.name_ = "Alice";      // Won't compile - private member!

Updating Records
~~~~~~~~~~~~~~~~

✅ **Do: Find, modify using setters, then update**

.. code-block:: cpp

   auto student = students.find(1001);
   if (student) {
       student->set_age(21);
       student->set_gpa(3.9);
       students.update(*student);
   }

❌ **Avoid: Modifying without updating**

.. code-block:: cpp

   auto student = students.find(1001);
   if (student) {
       student->set_age(21);
       // Forgot to call students.update(*student)!
       // Changes won't be persisted!
   }

Testing
-------

Write Unit Tests
~~~~~~~~~~~~~~~~

✅ **Do: Test your data models and queries**

.. code-block:: cpp

   #include "gtest/gtest.h"

   TEST(StudentTest, InsertAndQuery) {
       core::Database db(":memory:");
       auto& students = db.table<Student>("students");

       Student alice(1, "Alice", 20);
       students.insert(alice);

       auto found = students.find(1);
       ASSERT_TRUE(found.has_value());

       EXPECT_EQ(found->get_name(), "Alice");
       EXPECT_EQ(found->get_age(), 20);
   }

Use Fixtures for Complex Tests
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   class StudentDatabaseTest : public ::testing::Test {
   protected:
       std::unique_ptr<core::Database> db;
       core::Table<Student>* students;

       void SetUp() override {
           db = std::make_unique<core::Database>(":memory:");
           students = &db->table<Student>("students");

           // Seed test data
           students->insert(Student(1, "Alice", 20));
           students->insert(Student(2, "Bob", 22));
       }
   };

   TEST_F(StudentDatabaseTest, QueryByAge) {
       int count = 0;
       for (const auto& s : students->where(Student::age >= 21)) {
           count++;
       }
       EXPECT_EQ(count, 1);
   }

Common Anti-Patterns
--------------------

Anti-Pattern: Ignoring Type Safety
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

❌ **Avoid: Type mismatches**

.. code-block:: cpp

   // Compile error - good!
   students.where(Student::name >= 123);  // Can't compare string with int

✅ **Do: Respect the type system**

.. code-block:: cpp

   students.where(Student::name == "Alice");  // Correct types

Anti-Pattern: Premature Optimization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

❌ **Avoid: Over-indexing**

.. code-block:: cpp

   // Creating indexes on every field "just in case"
   auto& students = db.table<Student>("students")
       .add_index(Student::name, core::IndexType::Unique)
       .add_index(Student::age, core::IndexType::MultiValue)
       .add_index(Student::department, core::IndexType::MultiValue)
       .add_index(Student::gpa, core::IndexType::MultiValue);
   // ... etc

**Why is this bad?**

* Indexes consume memory
* Indexes slow down inserts and updates
* Most queries might not even use them

✅ **Do: Profile first, then optimize**

.. code-block:: cpp

   // Create indexes only for frequently queried fields
   auto& students = db.table<Student>("students")
       .add_index(Student::department, core::IndexType::MultiValue);

Anti-Pattern: Changing Primary Keys
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

❌ **Avoid: Modifying primary keys**

.. code-block:: cpp

   Student s(1, "Alice", 20);
   students.insert(s);

   s.set_student_id(2);  // Don't do this!
   students.update(s);    // Database now inconsistent!

✅ **Do: Delete and re-insert if needed**

.. code-block:: cpp

   students.remove(1);
   s.set_student_id(2);
   students.insert(s);

Debugging Tips
--------------

Check Record Existence
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   if (students.contains(123)) {
       std::cout << "Student exists" << std::endl;
   } else {
       std::cout << "Student not found" << std::endl;
   }

Count Records
~~~~~~~~~~~~~

.. code-block:: cpp

   std::cout << "Total students: " << students.size() << std::endl;

Print Query Results for Debugging
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   std::cout << "Finding CS students:" << std::endl;
   for (const auto& s : students.where(Student::department == "CS")) {
       std::cout << "  - " << s.get_name() << " (GPA: " << s.get_gpa() << ")" << std::endl;
   }

Learning Resources
------------------

Next Steps
~~~~~~~~~~

* :doc:`/tutorials/tutorial-01-first-database` - Build a complete application
* :doc:`/tutorials/tutorial-06-indexes-performance` - Learn performance optimization
* :doc:`/architecture/overview` - Understand how LearnQL works internally
* :doc:`/resources/examples` - Browse complete code examples

Further Reading
~~~~~~~~~~~~~~~

* **C++20 Concepts**: Understanding concepts helps you use LearnQL effectively
* **Expression Templates**: Learn about this powerful C++ technique
* **B-Tree Algorithms**: Understand how database indexes work

.. tip::
   The best way to learn is by doing! Start with the tutorials and build real projects. Don't worry about making mistakes - that's how you learn!

Summary
-------

**Key Takeaways:**

1. Use integer primary keys for best performance
2. Create indexes for frequently queried fields using ``.add_index()``
3. Always check optional returns before accessing (``find()`` returns ``std::optional``)
4. Use ``:memory:`` databases for testing
5. Compile in Release mode for production
6. Let RAII handle database cleanup
7. Use getters and setters (``get_name()``, ``set_name()``)
8. Embrace type safety - it catches bugs at compile-time
9. Write unit tests for your database code
10. Use ``where()`` with static Field objects (``Student::age >= 21``)

Following these best practices will help you write efficient, maintainable LearnQL code!
