Tutorial 2: Complete CRUD Operations
=====================================

In this tutorial, you'll master the complete lifecycle of database records: Create, Read, Update, and Delete. You'll build a student management system using LearnQL's actual API with ``find()``, ``update()``, ``remove()``, and ``contains()`` methods.

**Time**: 25 minutes
**Level**: Beginner
**Prerequisites**: Completed :doc:`tutorial-01-first-database`

What We'll Build
----------------

A student management system with:

* Student records with GPA tracking
* Adding new students (Create)
* Finding students by ID (Read)
* Updating student information (Update)
* Removing graduated students (Delete)
* Checking if students exist before operations

Step 1: Define the Student Model
---------------------------------

Create a new file ``student_manager.cpp``:

.. code-block:: cpp

   #include <learnql/LearnQL.hpp>
   #include <iostream>
   #include <string>
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

   public:
       Student() = default;

       Student(int id, const std::string& n, const std::string& d, int a, double g)
           : student_id_(id), name_(n), department_(d), age_(a), gpa_(g) {}
   };

**Understanding the model:**

* ``student_id`` is the primary key (marked with ``PK``)
* ``gpa`` uses ``double`` for decimal precision
* All properties automatically generate getters and setters
* The property macros create static Field objects for queries

Step 2: Create Operation - Adding Students
--------------------------------------------

Let's create a function to add students using ``insert()``:

.. code-block:: cpp

   void add_student(core::Table<Student>& students, const Student& new_student) {
       std::cout << "\n=== Adding New Student ===" << std::endl;

       try {
           students.insert(new_student);
           std::cout << "✓ Student added: " << new_student.get_name() << std::endl;
       } catch (const std::exception& e) {
           std::cerr << "✗ Error adding student: " << e.what() << std::endl;
       }
   }

   void add_sample_students(core::Table<Student>& students) {
       std::cout << "Adding sample students..." << std::endl;

       students.insert(Student(1001, "Alice Johnson", "CS", 20, 3.8));
       students.insert(Student(1002, "Bob Smith", "CS", 21, 3.5));
       students.insert(Student(1003, "Carol White", "Math", 19, 3.9));
       students.insert(Student(1004, "David Brown", "Physics", 22, 3.2));
       students.insert(Student(1005, "Eve Davis", "CS", 20, 3.7));

       std::cout << "✓ Inserted " << students.size() << " students" << std::endl;
   }

Step 3: Read Operation - Finding Students
------------------------------------------

Finding by Primary Key
~~~~~~~~~~~~~~~~~~~~~~

The LearnQL API provides ``find()`` which returns ``std::optional<T>``:

.. code-block:: cpp

   void find_student_by_id(core::Table<Student>& students, int student_id) {
       std::cout << "\n=== Finding Student #" << student_id << " ===" << std::endl;

       // find() returns std::optional<Student>
       auto student = students.find(student_id);

       if (student) {
           // Student found
           std::cout << "✓ Found student:" << std::endl;
           std::cout << "  Name: " << student->get_name() << std::endl;
           std::cout << "  Age: " << student->get_age() << std::endl;
           std::cout << "  Department: " << student->get_department() << std::endl;
           std::cout << "  GPA: " << std::fixed << std::setprecision(2)
                     << student->get_gpa() << std::endl;
       } else {
           std::cout << "✗ Student not found!" << std::endl;
       }
   }

**Understanding the code:**

* ``find(primary_key)`` returns ``std::optional<Student>``
* Check if student exists with ``if (student)``
* Use ``student->get_field()`` to access properties
* All access is through auto-generated getter methods

Finding with Queries
~~~~~~~~~~~~~~~~~~~~

You can also search using the query API with static Fields:

.. code-block:: cpp

   void find_students_by_department(core::Table<Student>& students,
                                    const std::string& department) {
       std::cout << "\n=== Students in " << department << " ===" << std::endl;

       // Use static Field for queries
       auto results = students.where(Student::department == department);

       if (results.empty()) {
           std::cout << "No students found in this department." << std::endl;
           return;
       }

       for (const auto& student : results) {
           std::cout << "  - " << student.get_name()
                     << " (GPA: " << std::fixed << std::setprecision(1)
                     << student.get_gpa() << ")" << std::endl;
       }
   }

Finding with Lambda Predicates
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For complex filtering, use ``find_if()`` with lambda predicates:

.. code-block:: cpp

   void find_high_achievers(core::Table<Student>& students, double min_gpa) {
       std::cout << "\n=== High Achievers (GPA >= " << min_gpa << ") ===" << std::endl;

       // Use find_if with lambda predicate
       auto results = students.find_if([min_gpa](const Student& s) {
           return s.get_gpa() >= min_gpa && s.get_age() >= 21;
       });

       for (const auto& student : results) {
           std::cout << "  - " << student.get_name()
                     << " (GPA: " << student.get_gpa() << ")" << std::endl;
       }
   }

Step 4: Update Operation - Modifying Records
---------------------------------------------

To update a record:

1. Find the record with ``find()``
2. Modify its fields using setters
3. Save it back with ``update()``

Basic Update Example
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void update_student_gpa(core::Table<Student>& students, int student_id,
                           double new_gpa) {
       std::cout << "\n=== Updating Student #" << student_id << " ===" << std::endl;

       // Step 1: Find the student
       auto student = students.find(student_id);

       if (!student) {
           std::cout << "✗ Student not found!" << std::endl;
           return;
       }

       // Step 2: Modify the field using setter
       double old_gpa = student->get_gpa();
       student->set_gpa(new_gpa);

       // Step 3: Save the updated record
       students.update(*student);

       std::cout << "✓ Updated " << student->get_name()
                 << " GPA: " << old_gpa << " → " << new_gpa << std::endl;
   }

Updating Multiple Fields
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void update_student_info(core::Table<Student>& students, int student_id) {
       auto student = students.find(student_id);

       if (!student) {
           std::cout << "Student not found!" << std::endl;
           return;
       }

       // Update multiple fields using setters
       student->set_age(21);  // Birthday!
       student->set_gpa(3.9);  // Improved grades
       student->set_department("Computer Science");

       students.update(*student);

       std::cout << "✓ Student info updated successfully!" << std::endl;
   }

.. note::
   You cannot update the primary key. The primary key is immutable once a record is inserted.

Step 5: Delete Operation - Removing Records
--------------------------------------------

Deleting a Single Record
~~~~~~~~~~~~~~~~~~~~~~~~

Use ``remove()`` with the primary key:

.. code-block:: cpp

   void remove_student(core::Table<Student>& students, int student_id) {
       std::cout << "\n=== Removing Student #" << student_id << " ===" << std::endl;

       // Find the student first to get their name
       auto student = students.find(student_id);

       if (!student) {
           std::cout << "✗ Student not found!" << std::endl;
           return;
       }

       std::string name = student->get_name();

       // Delete the record by primary key
       students.remove(student_id);

       std::cout << "✓ Removed student: " << name << std::endl;
   }

Deleting Multiple Records
~~~~~~~~~~~~~~~~~~~~~~~~~~

To delete multiple records, collect their IDs first:

.. code-block:: cpp

   void remove_graduated_students(core::Table<Student>& students, double min_gpa) {
       std::cout << "\n=== Removing Graduated Students (GPA >= "
                 << min_gpa << ") ===" << std::endl;

       // Find all students who graduated
       auto results = students.find_if([min_gpa](const Student& s) {
           return s.get_gpa() >= min_gpa;
       });

       // Collect IDs to delete
       std::vector<int> to_delete;
       for (const auto& student : results) {
           to_delete.push_back(student.get_student_id());
           std::cout << "  Graduating: " << student.get_name()
                     << " (GPA: " << student.get_gpa() << ")" << std::endl;
       }

       // Delete all collected records
       for (int id : to_delete) {
           students.remove(id);
       }

       std::cout << "✓ Removed " << to_delete.size() << " students" << std::endl;
   }

.. warning::
   Don't modify a table while iterating over it! Always collect IDs first, then delete them in a separate loop.

Step 6: Checking Record Existence
----------------------------------

Use ``contains()`` to check if a record exists:

.. code-block:: cpp

   bool student_exists(core::Table<Student>& students, int student_id) {
       return students.contains(student_id);
   }

   void safe_update_student(core::Table<Student>& students, int student_id,
                            double new_gpa) {
       if (!students.contains(student_id)) {
           std::cout << "✗ Cannot update: student #"
                     << student_id << " does not exist" << std::endl;
           return;
       }

       // Proceed with update
       update_student_gpa(students, student_id, new_gpa);
   }

   void demonstrate_existence_checks(core::Table<Student>& students) {
       std::cout << "\n=== Checking Student Existence ===" << std::endl;

       std::cout << "Student 1001 exists: "
                 << (students.contains(1001) ? "Yes" : "No") << std::endl;
       std::cout << "Student 9999 exists: "
                 << (students.contains(9999) ? "Yes" : "No") << std::endl;
   }

Step 7: Complete Program
-------------------------

Here's the complete student management system:

.. code-block:: cpp

   #include <learnql/LearnQL.hpp>
   #include <iostream>
   #include <string>
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

   public:
       Student() = default;

       Student(int id, const std::string& n, const std::string& d, int a, double g)
           : student_id_(id), name_(n), department_(d), age_(a), gpa_(g) {}
   };

   void add_sample_students(core::Table<Student>& students) {
       std::cout << "Adding sample students..." << std::endl;

       students.insert(Student(1001, "Alice Johnson", "CS", 20, 3.8));
       students.insert(Student(1002, "Bob Smith", "CS", 21, 3.5));
       students.insert(Student(1003, "Carol White", "Math", 19, 3.9));
       students.insert(Student(1004, "David Brown", "Physics", 22, 3.2));
       students.insert(Student(1005, "Eve Davis", "CS", 20, 3.7));

       std::cout << "Added " << students.size() << " students" << std::endl;
   }

   void display_all_students(core::Table<Student>& students) {
       std::cout << "\n=== All Students ===" << std::endl;
       std::cout << std::string(80, '=') << std::endl;

       for (const auto& s : students) {
           std::cout << "ID: " << s.get_student_id() << " | "
                     << s.get_name() << " | "
                     << s.get_department() << " | "
                     << "GPA: " << std::fixed << std::setprecision(1)
                     << s.get_gpa() << std::endl;
       }

       std::cout << "\nTotal: " << students.size() << " students" << std::endl;
   }

   void update_student_demo(core::Table<Student>& students) {
       std::cout << "\n=== Update Demo: Improving Bob's GPA ===" << std::endl;

       int student_id = 1002;
       auto student = students.find(student_id);

       if (student) {
           std::cout << "Before: " << student->get_name()
                     << " has GPA " << student->get_gpa() << std::endl;

           student->set_gpa(3.8);
           students.update(*student);

           std::cout << "After: " << student->get_name()
                     << " has GPA " << student->get_gpa() << std::endl;
       }
   }

   void delete_student_demo(core::Table<Student>& students) {
       std::cout << "\n=== Delete Demo: Removing student #1004 ===" << std::endl;

       int student_id = 1004;
       auto student = students.find(student_id);

       if (student) {
           std::cout << "Removing: " << student->get_name() << std::endl;
           students.remove(student_id);
           std::cout << "✓ Student removed" << std::endl;
       }
   }

   void find_and_display_student(core::Table<Student>& students, int student_id) {
       std::cout << "\n=== Finding Student #" << student_id << " ===" << std::endl;

       auto student = students.find(student_id);

       if (student) {
           std::cout << "✓ Found: " << student->get_name() << std::endl;
           std::cout << "  Age: " << student->get_age() << std::endl;
           std::cout << "  Department: " << student->get_department() << std::endl;
           std::cout << "  GPA: " << std::fixed << std::setprecision(1)
                     << student->get_gpa() << std::endl;
       } else {
           std::cout << "✗ Student not found!" << std::endl;
       }
   }

   int main() {
       try {
           core::Database db("students.db");
           auto& students = db.table<Student>("students");

           // Only add if table is empty
           if (students.size() == 0) {
               add_sample_students(students);
           }

           // Display all students
           display_all_students(students);

           // Demonstrate UPDATE
           update_student_demo(students);

           // Demonstrate READ by primary key
           find_and_display_student(students, 1002);

           // Display after update
           display_all_students(students);

           // Demonstrate DELETE
           delete_student_demo(students);

           // Display final state
           display_all_students(students);

           return 0;
       } catch (const std::exception& e) {
           std::cerr << "Error: " << e.what() << std::endl;
           return 1;
       }
   }

Build and run:

.. code-block:: bash

   mkdir build && cd build
   cmake ..
   cmake --build .
   ./student_manager

Expected Output
---------------

.. code-block:: text

   Adding sample students...
   Added 5 students

   === All Students ===
   ================================================================================
   ID: 1001 | Alice Johnson | CS | GPA: 3.8
   ID: 1002 | Bob Smith | CS | GPA: 3.5
   ID: 1003 | Carol White | Math | GPA: 3.9
   ID: 1004 | David Brown | Physics | GPA: 3.2
   ID: 1005 | Eve Davis | CS | GPA: 3.7

   Total: 5 students

   === Update Demo: Improving Bob's GPA ===
   Before: Bob Smith has GPA 3.5
   After: Bob Smith has GPA 3.8

   === Finding Student #1002 ===
   ✓ Found: Bob Smith
     Age: 21
     Department: CS
     GPA: 3.8

   ... (updated list) ...

   === Delete Demo: Removing student #1004 ===
   Removing: David Brown
   ✓ Student removed

   Total: 4 students

What You Learned
----------------

✅ Creating records with ``insert()``

✅ Finding records with ``find()`` returning ``std::optional<T>``

✅ Updating records with ``update()``

✅ Deleting records with ``remove()``

✅ Using ``contains()`` to check existence

✅ Accessing properties with auto-generated getters like ``get_name()``

✅ Modifying properties with auto-generated setters like ``set_gpa()``

✅ Using static Field objects for queries (``Student::department``)

✅ Using lambda predicates with ``find_if()``

✅ Safe patterns for modifying and deleting records

Exercises
---------

Try these challenges to practice CRUD operations:

1. **Add a birthday function**

   .. code-block:: cpp

      void celebrate_birthday(core::Table<Student>& students, int student_id);
      // Increment the student's age by 1

2. **Implement batch update**

   .. code-block:: cpp

      void update_department_for_all(core::Table<Student>& students,
                                      const std::string& old_dept,
                                      const std::string& new_dept);
      // Change department for all matching students

3. **Create a copy function**

   .. code-block:: cpp

      void duplicate_student(core::Table<Student>& students,
                            int source_id, int new_id);
      // Copy student with a new ID

4. **Implement conditional delete**

   .. code-block:: cpp

      void remove_students_below_gpa(core::Table<Student>& students,
                                     double min_gpa);
      // Remove all students with GPA below the threshold

5. **Add validation**

   Create a function that validates student data before updating:

   .. code-block:: cpp

      bool is_valid_gpa(double gpa);  // Must be 0.0 - 4.0
      void safe_update_gpa(core::Table<Student>& students,
                          int student_id, double new_gpa);

Next Steps
----------

Continue to :doc:`tutorial-03-query-dsl` to master the Query DSL with static Field objects, complex expressions, logical operators, and advanced filtering techniques.

.. tip::
   Practice these CRUD patterns - they're the foundation of all database operations! Try creating your own management system for a different domain (employees, products, courses, etc.).
