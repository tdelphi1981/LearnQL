Tutorial 4: Joins and Relationships
====================================

In this tutorial, you'll learn how to work with related data across multiple tables using LearnQL's built-in join operations. You'll build a university course enrollment system that demonstrates inner joins and left joins for combining student, course, and enrollment data.

**Time**: 30 minutes
**Level**: Intermediate
**Prerequisites**: Completed :doc:`tutorial-03-query-dsl`

What We'll Build
----------------

A university enrollment system with:

* Students table with personal information
* Courses table with course details
* Enrollments table linking students to courses (many-to-many relationship)
* Inner join to find students with their enrollments
* Left join to include students without enrollments
* Multi-table join aggregations for calculating total credits

Understanding Relationships
---------------------------

Database Relationships
~~~~~~~~~~~~~~~~~~~~~~

**One-to-Many**: One student has many enrollments

.. code-block:: text

   Student (1) ----< Enrollment (many)

**Many-to-Many**: Students enroll in courses, courses have students

.. code-block:: text

   Student (many) >---- Enrollment ----< Course (many)
                       (junction table)

**Foreign Key**: A field that references another table's primary key

.. code-block:: text

   Students Table          Enrollments Table       Courses Table
   ┌────────────┐         ┌─────────────────┐     ┌────────────┐
   │ student_id │←────────│ student_id (FK) │     │ course_code│
   │ name       │         │ course_code(FK) │─────→│ name       │
   │ department │         │ grade           │     │ credits    │
   └────────────┘         └─────────────────┘     └────────────┘

.. note::
   LearnQL provides type-safe join operations through the ``query::Join`` class. Joins are performed in-memory using lambda functions to specify join keys.

Step 1: Define the Data Models
-------------------------------

Create ``enrollment_system.cpp``:

.. code-block:: cpp

   #include "learnql/LearnQL.hpp"
   #include <iostream>
   #include <string>
   #include <iomanip>
   #include <unordered_map>
   #include <vector>

   using namespace learnql;

   // Student model
   struct Student {
       LEARNQL_PROPERTY(int, student_id);
       LEARNQL_PROPERTY(std::string, name);
       LEARNQL_PROPERTY(std::string, department);
       LEARNQL_PROPERTY(int, age);
       LEARNQL_PROPERTY(double, gpa);
   };

   // Course model
   struct Course {
       LEARNQL_PROPERTY(std::string, course_code);  // Primary key (string)
       LEARNQL_PROPERTY(std::string, name);
       LEARNQL_PROPERTY(std::string, department);
       LEARNQL_PROPERTY(int, credits);
   };

   // Enrollment model (junction table)
   struct Enrollment {
       LEARNQL_PROPERTY(int, enrollment_id);
       LEARNQL_PROPERTY(int, student_id);           // Foreign key → Student
       LEARNQL_PROPERTY(std::string, course_code);  // Foreign key → Course
       LEARNQL_PROPERTY(std::string, semester);
       LEARNQL_PROPERTY(std::string, grade);
   };

**Key points:**

* ``Student.student_id`` is referenced by ``Enrollment.student_id``
* ``Course.course_code`` is referenced by ``Enrollment.course_code``
* ``Enrollment`` is the junction table enabling many-to-many relationships
* Primary keys can be any type (int for Student, string for Course)

Step 2: Adding Sample Data
---------------------------

.. code-block:: cpp

   void populate_data(Database& db) {
       auto& students = db.table<Student>("students");
       auto& courses = db.table<Course>("courses");
       auto& enrollments = db.table<Enrollment>("enrollments");

       // Add students
       students.insert({1001, "Alice Johnson", "CS", 20, 3.8});
       students.insert({1002, "Bob Smith", "Math", 21, 3.5});
       students.insert({1003, "Carol White", "CS", 19, 3.9});
       students.insert({1004, "David Brown", "Physics", 22, 3.6});
       students.insert({1005, "Eve Davis", "CS", 20, 3.7});
       students.insert({1006, "Frank Wilson", "Math", 23, 3.4});  // No enrollments

       std::cout << "Added " << students.size() << " students\n";

       // Add courses
       courses.insert({"CS101", "Intro to Programming", "CS", 4});
       courses.insert({"CS201", "Data Structures", "CS", 4});
       courses.insert({"MATH101", "Calculus I", "Math", 4});
       courses.insert({"PHYS101", "Physics I", "Physics", 3});
       courses.insert({"CS301", "Algorithms", "CS", 3});

       std::cout << "Added " << courses.size() << " courses\n";

       // Add enrollments
       enrollments.insert({1, 1001, "CS101", "Fall 2024", "A"});
       enrollments.insert({2, 1001, "CS201", "Fall 2024", "A-"});
       enrollments.insert({3, 1001, "MATH101", "Fall 2024", "B+"});

       enrollments.insert({4, 1002, "MATH101", "Fall 2024", "A"});
       enrollments.insert({5, 1002, "CS301", "Fall 2024", "B"});

       enrollments.insert({6, 1003, "CS101", "Fall 2024", "A"});
       enrollments.insert({7, 1003, "CS201", "Fall 2024", "A"});

       enrollments.insert({8, 1004, "PHYS101", "Fall 2024", "B+"});

       enrollments.insert({9, 1005, "CS201", "Fall 2024", "A"});
       enrollments.insert({10, 1005, "CS301", "Fall 2024", "A-"});

       std::cout << "Added " << enrollments.size() << " enrollments\n\n";
   }

Step 3: Inner Join - Students with Enrollments
-----------------------------------------------

An **inner join** returns only records where the join key matches in both tables.

Basic Inner Join
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void demonstrate_inner_join(Table<Student>& students,
                               Table<Enrollment>& enrollments) {
       std::cout << "=== Inner Join: Students with Enrollments ===\n";
       std::cout << std::string(80, '-') << "\n";

       // Inner join on student_id
       auto student_enrollments = query::Join<Student, Enrollment>::inner_join(
           students,
           enrollments,
           [](const Student& s) { return s.get_student_id(); },
           [](const Enrollment& e) { return e.get_student_id(); }
       );

       // Display results
       for (const auto& [student, enrollment_opt] : student_enrollments) {
           if (enrollment_opt) {
               std::cout << std::setw(20) << std::left << student.get_name() << " | "
                         << "Course: " << std::setw(8) << enrollment_opt->get_course_code() << " | "
                         << "Grade: " << enrollment_opt->get_grade() << "\n";
           }
       }
   }

**How it works:**

1. ``query::Join<Student, Enrollment>::inner_join()`` performs the join
2. First lambda extracts the join key from Student (``student_id``)
3. Second lambda extracts the join key from Enrollment (``student_id``)
4. Returns pairs of ``(Student, std::optional<Enrollment>)``
5. Only students with at least one enrollment are included

**Expected output:**

.. code-block:: text

   Alice Johnson        | Course: CS101    | Grade: A
   Alice Johnson        | Course: CS201    | Grade: A-
   Alice Johnson        | Course: MATH101  | Grade: B+
   Bob Smith            | Course: MATH101  | Grade: A
   Bob Smith            | Course: CS301    | Grade: B
   ...

.. note::
   Frank Wilson (student 1006) won't appear because he has no enrollments. Use a left join to include all students.

Step 4: Left Join - All Students
---------------------------------

A **left join** returns all records from the left table, even without matches in the right table.

Left Join Implementation
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void demonstrate_left_join(Table<Student>& students,
                              Table<Enrollment>& enrollments) {
       std::cout << "\n=== Left Join: All Students (including unenrolled) ===\n";
       std::cout << std::string(80, '-') << "\n";

       // Left join on student_id
       auto all_student_enrollments = query::Join<Student, Enrollment>::left_join(
           students,
           enrollments,
           [](const Student& s) { return s.get_student_id(); },
           [](const Enrollment& e) { return e.get_student_id(); }
       );

       // Group by student to show all their courses
       std::unordered_map<int, std::vector<std::string>> student_courses;
       for (const auto& [student, enrollment_opt] : all_student_enrollments) {
           if (enrollment_opt) {
               student_courses[student.get_student_id()].push_back(
                   enrollment_opt->get_course_code()
               );
           }
       }

       // Display each student
       for (const auto& student : students) {
           std::cout << std::setw(20) << std::left << student.get_name() << " | ";

           if (student_courses.count(student.get_student_id())) {
               std::cout << "Courses: ";
               const auto& courses = student_courses[student.get_student_id()];
               for (size_t i = 0; i < courses.size(); ++i) {
                   if (i > 0) std::cout << ", ";
                   std::cout << courses[i];
               }
           } else {
               std::cout << "No enrollments";
           }
           std::cout << "\n";
       }
   }

**Expected output:**

.. code-block:: text

   Alice Johnson        | Courses: CS101, CS201, MATH101
   Bob Smith            | Courses: MATH101, CS301
   Carol White          | Courses: CS101, CS201
   David Brown          | Courses: PHYS101
   Eve Davis            | Courses: CS201, CS301
   Frank Wilson         | No enrollments

Step 5: Multi-Table Joins with Aggregation
-------------------------------------------

Join students → enrollments → courses to calculate total credits.

Calculate Total Credits per Student
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void calculate_student_credits(Table<Student>& students,
                                  Table<Enrollment>& enrollments,
                                  Table<Course>& courses) {
       std::cout << "\n=== Total Credits per Student ===\n";
       std::cout << std::string(80, '-') << "\n";

       // Step 1: Join students with enrollments
       auto student_enrollments = query::Join<Student, Enrollment>::inner_join(
           students,
           enrollments,
           [](const Student& s) { return s.get_student_id(); },
           [](const Enrollment& e) { return e.get_student_id(); }
       );

       // Step 2: Extract student-course pairs
       struct StudentCourse {
           int student_id;
           std::string student_name;
           std::string course_code;
       };

       std::vector<StudentCourse> student_courses;
       for (const auto& [student, enrollment_opt] : student_enrollments) {
           if (enrollment_opt) {
               student_courses.push_back({
                   student.get_student_id(),
                   student.get_name(),
                   enrollment_opt->get_course_code()
               });
           }
       }

       // Step 3: Look up course credits and aggregate
       std::unordered_map<int, std::pair<std::string, int>> student_credits;

       for (const auto& sc : student_courses) {
           auto course = courses.find(sc.course_code);
           if (course) {
               student_credits[sc.student_id].first = sc.student_name;
               student_credits[sc.student_id].second += course->get_credits();
           }
       }

       // Step 4: Display results
       for (const auto& [id, name_credits] : student_credits) {
           std::cout << std::setw(20) << std::left << name_credits.first << " | "
                     << "Total Credits: " << name_credits.second << "\n";
       }
   }

**How it works:**

1. Join students with enrollments on ``student_id``
2. Extract student-course pairs into a temporary structure
3. Look up each course to get credits (using ``courses.find()``)
4. Aggregate credits per student using a map
5. Display the totals

**Expected output:**

.. code-block:: text

   Alice Johnson        | Total Credits: 12
   Bob Smith            | Total Credits: 7
   Carol White          | Total Credits: 8
   David Brown          | Total Credits: 3
   Eve Davis            | Total Credits: 7

Step 6: Finding Unenrolled Students
------------------------------------

Using left join to identify students without enrollments.

.. code-block:: cpp

   void find_unenrolled_students(Table<Student>& students,
                                 Table<Enrollment>& enrollments) {
       std::cout << "\n=== Students Without Enrollments ===\n";
       std::cout << std::string(80, '-') << "\n";

       auto all_students = query::Join<Student, Enrollment>::left_join(
           students,
           enrollments,
           [](const Student& s) { return s.get_student_id(); },
           [](const Enrollment& e) { return e.get_student_id(); }
       );

       // Collect students with no enrollments
       std::set<int> students_with_enrollments;
       for (const auto& [student, enrollment_opt] : all_students) {
           if (enrollment_opt) {
               students_with_enrollments.insert(student.get_student_id());
           }
       }

       // Display students not in the set
       bool found_any = false;
       for (const auto& student : students) {
           if (students_with_enrollments.find(student.get_student_id()) ==
               students_with_enrollments.end()) {
               std::cout << "  - " << student.get_name()
                         << " (" << student.get_department() << ")\n";
               found_any = true;
           }
       }

       if (!found_any) {
           std::cout << "  All students are enrolled in at least one course.\n";
       }
   }

Step 7: Course Roster Report
-----------------------------

Show all students enrolled in a specific course.

.. code-block:: cpp

   void print_course_roster(Table<Student>& students,
                           Table<Enrollment>& enrollments,
                           Table<Course>& courses,
                           const std::string& course_code) {
       std::cout << "\n=== Course Roster: " << course_code << " ===\n";

       // Get course info
       auto course = courses.find(course_code);
       if (!course) {
           std::cout << "Course not found!\n";
           return;
       }

       std::cout << "Course: " << course->get_name() << "\n";
       std::cout << "Department: " << course->get_department() << "\n";
       std::cout << "Credits: " << course->get_credits() << "\n";
       std::cout << std::string(80, '-') << "\n";

       // Join students with enrollments
       auto student_enrollments = query::Join<Student, Enrollment>::inner_join(
           students,
           enrollments,
           [](const Student& s) { return s.get_student_id(); },
           [](const Enrollment& e) { return e.get_student_id(); }
       );

       // Filter for this course and display
       int count = 0;
       for (const auto& [student, enrollment_opt] : student_enrollments) {
           if (enrollment_opt && enrollment_opt->get_course_code() == course_code) {
               count++;
               std::cout << count << ". "
                         << std::setw(20) << std::left << student.get_name()
                         << " (" << student.get_department()
                         << ", GPA: " << std::fixed << std::setprecision(2)
                         << student.get_gpa() << ")"
                         << " - Grade: " << enrollment_opt->get_grade() << "\n";
           }
       }

       std::cout << std::string(80, '-') << "\n";
       std::cout << "Total enrolled: " << count << " students\n";
   }

Step 8: Complete Enrollment System
-----------------------------------

.. code-block:: cpp

   int main() {
       try {
           Database db("university.db");

           // Get table references
           auto& students = db.table<Student>("students");
           auto& courses = db.table<Course>("courses");
           auto& enrollments = db.table<Enrollment>("enrollments");

           // Populate data if empty
           if (students.size() == 0) {
               populate_data(db);
           }

           // Demonstrate join operations
           demonstrate_inner_join(students, enrollments);
           demonstrate_left_join(students, enrollments);

           // Multi-table aggregation
           calculate_student_credits(students, enrollments, courses);

           // Analysis queries
           find_unenrolled_students(students, enrollments);
           print_course_roster(students, enrollments, courses, "CS201");

           return 0;
       } catch (const std::exception& e) {
           std::cerr << "Error: " << e.what() << std::endl;
           return 1;
       }
   }

Join Patterns Summary
---------------------

Inner Join
~~~~~~~~~~

Returns only matching records from both tables.

.. code-block:: cpp

   auto joined = query::Join<LeftTable, RightTable>::inner_join(
       left_table,
       right_table,
       [](const LeftTable& l) { return l.get_join_key(); },
       [](const RightTable& r) { return r.get_join_key(); }
   );

Left Join
~~~~~~~~~

Returns all records from left table, with nullopt for non-matching right records.

.. code-block:: cpp

   auto joined = query::Join<LeftTable, RightTable>::left_join(
       left_table,
       right_table,
       [](const LeftTable& l) { return l.get_join_key(); },
       [](const RightTable& r) { return r.get_join_key(); }
   );

Join Result Type
~~~~~~~~~~~~~~~~

Both joins return:

.. code-block:: cpp

   std::vector<std::pair<LeftTable, std::optional<RightTable>>>

* First element is always from left table
* Second element is ``std::optional<RightTable>``
* For inner join, optional always has a value
* For left join, optional is empty when no match exists

What You Learned
----------------

✅ Understanding one-to-many and many-to-many relationships

✅ Creating junction tables for many-to-many relationships

✅ Using ``query::Join::inner_join()`` for matching records only

✅ Using ``query::Join::left_join()`` to include all left records

✅ Extracting join keys with lambda functions

✅ Working with ``std::optional`` in join results

✅ Multi-table joins for complex aggregations

✅ Grouping join results for reporting

✅ Finding unmatched records with left joins

Exercises
---------

Try these challenges to master joins:

1. **Student transcript report**

   Create a function that displays a student's complete transcript with course names, credits, and grades. Calculate their semester GPA.

2. **Department course offerings**

   Find all courses offered by a specific department and show how many students are enrolled in each.

3. **Grade distribution**

   For a given course, show the distribution of grades (how many A's, B's, etc.).

4. **Prerequisites check**

   Add a ``prerequisites`` field to Course. Write a function to check if a student has completed the prerequisites before enrolling.

5. **Cross-department enrollments**

   Find students taking courses outside their major department.

Next Steps
----------

Continue to :doc:`tutorial-05-aggregations-groupby` to learn about LearnQL's built-in aggregation functions (count_by, average_by, sum_by) and GROUP BY operations for advanced data analysis.

.. tip::
   When designing database schemas, always identify relationships first. Use junction tables for many-to-many relationships, and use foreign keys (just regular fields in LearnQL) to link related data.
