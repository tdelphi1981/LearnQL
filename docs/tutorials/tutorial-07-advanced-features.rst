Tutorial 7: Advanced Features and C++20 Integration
===================================================

In this tutorial, you'll explore LearnQL's advanced features including lambda predicates with ``find_if``, C++20 ranges integration, and best practices for building sophisticated database applications. You'll learn how to combine multiple query techniques to solve complex real-world problems.

**Time**: 25 minutes
**Level**: Advanced
**Prerequisites**: Completed :doc:`tutorial-06-indexes-performance`

What We'll Build
----------------

An advanced query system featuring:

* Lambda predicates with ``find_if`` for custom filtering
* C++20 ranges for functional-style data transformation
* Complex multi-criteria queries
* Combining indexes with lambda filters
* Performance optimization patterns
* Best practices for production systems

Understanding Advanced Queries
-------------------------------

Query Types in LearnQL
~~~~~~~~~~~~~~~~~~~~~~

LearnQL provides multiple ways to query data:

1. **Index-based**: Fast O(log n) lookups

   .. code-block:: cpp

      auto student = table.find_by(Student::name, "Alice");

2. **Lambda predicates**: Flexible custom filtering

   .. code-block:: cpp

      auto students = table.find_if([](const Student& s) {
          return s.get_gpa() > 3.5 && s.get_age() < 21;
      });

3. **C++20 ranges**: Functional transformations

   .. code-block:: cpp

      auto names = students | views::transform([](const Student& s) {
          return s.get_name();
      });

.. note::
   Choose the right tool: Use indexes for simple equality queries, lambda predicates for complex conditions, and ranges for post-query transformations.

Step 1: Define the Student Model
---------------------------------

Create ``advanced_queries.cpp``:

.. code-block:: cpp

   #include "learnql/LearnQL.hpp"
   #include <iostream>
   #include <string>
   #include <ranges>
   #include <algorithm>
   #include <iomanip>

   using namespace learnql;

   struct Student {
       LEARNQL_PROPERTY(int, student_id);
       LEARNQL_PROPERTY(std::string, name);
       LEARNQL_PROPERTY(std::string, department);
       LEARNQL_PROPERTY(int, age);
       LEARNQL_PROPERTY(double, gpa);
   };

   void populate_students(Table<Student>& students) {
       students.insert({1001, "Alice Johnson", "CS", 20, 3.8});
       students.insert({1002, "Bob Smith", "Math", 21, 3.5});
       students.insert({1003, "Carol White", "CS", 19, 3.9});
       students.insert({1004, "David Brown", "Physics", 22, 3.6});
       students.insert({1005, "Eve Davis", "CS", 20, 3.7});
       students.insert({1006, "Frank Wilson", "Math", 23, 3.4});
       students.insert({1007, "Grace Lee", "CS", 19, 4.0});
       students.insert({1008, "Henry Taylor", "Physics", 21, 3.8});
       students.insert({1009, "Ivy Chen", "Math", 20, 3.9});
       students.insert({1010, "Jack Miller", "CS", 22, 3.3});

       std::cout << "Populated " << students.size() << " students\n\n";
   }

Step 2: Lambda Predicates with find_if
---------------------------------------

The ``find_if`` method lets you define custom filtering logic with lambda expressions.

Basic Lambda Filtering
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void basic_lambda_filtering(Table<Student>& students) {
       std::cout << "=== Basic Lambda Filtering ===\n";
       std::cout << std::string(60, '-') << "\n";

       // Find CS students with GPA > 3.5
       auto cs_high_gpa = students.find_if([](const Student& s) {
           return s.get_department() == "CS" && s.get_gpa() > 3.5;
       });

       std::cout << "CS students with GPA > 3.5:\n";
       for (const auto& student : cs_high_gpa) {
           std::cout << "  - " << std::setw(20) << std::left << student.get_name()
                     << " GPA: " << std::fixed << std::setprecision(2)
                     << student.get_gpa() << "\n";
       }
   }

**How it works:**

1. ``find_if`` takes a lambda predicate: ``bool(const Student&)``
2. Iterates through all records (uses batched loading internally)
3. Returns ``std::vector<Student>`` with matching records
4. Predicate can contain any C++ logic

Complex Multi-Criteria Queries
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void complex_criteria(Table<Student>& students) {
       std::cout << "\n=== Complex Multi-Criteria Query ===\n";
       std::cout << std::string(60, '-') << "\n";

       // Elite students: Young CS majors with high GPA
       auto elite_students = students.find_if([](const Student& s) {
           return s.get_department() == "CS"
               && s.get_age() <= 20
               && s.get_gpa() >= 3.7;
       });

       std::cout << "Elite CS students (age ≤ 20, GPA ≥ 3.7):\n";
       for (const auto& student : elite_students) {
           std::cout << "  - " << std::setw(20) << std::left << student.get_name()
                     << " Age: " << student.get_age()
                     << ", GPA: " << std::fixed << std::setprecision(2)
                     << student.get_gpa() << "\n";
       }
   }

**Expected output:**

.. code-block:: text

   Elite CS students (age ≤ 20, GPA ≥ 3.7):
     - Alice Johnson       Age: 20, GPA: 3.80
     - Carol White         Age: 19, GPA: 3.90
     - Grace Lee           Age: 19, GPA: 4.00

Step 3: Combining Indexes with Lambda Filters
----------------------------------------------

For optimal performance, use indexes to narrow down results, then apply lambda filters.

Two-Stage Query Optimization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void optimized_two_stage_query(Table<Student>& students) {
       std::cout << "\n=== Optimized Two-Stage Query ===\n";
       std::cout << std::string(60, '-') << "\n";

       // Stage 1: Use index to get CS students (fast)
       auto cs_students = students.find_all_by(Student::department,
                                               std::string("CS"));

       std::cout << "Stage 1 (index): Found " << cs_students.size()
                 << " CS students\n";

       // Stage 2: Apply lambda filter (in-memory, on smaller dataset)
       std::vector<Student> young_cs_students;
       for (const auto& s : cs_students) {
           if (s.get_age() < 21 && s.get_gpa() > 3.5) {
               young_cs_students.push_back(s);
           }
       }

       std::cout << "Stage 2 (filter): Filtered to "
                 << young_cs_students.size() << " students\n\n";

       for (const auto& student : young_cs_students) {
           std::cout << "  - " << student.get_name() << "\n";
       }

       std::cout << "\nPerformance: O(log n) + O(k) where k = CS students\n";
       std::cout << "Much faster than O(n) full table scan!\n";
   }

**Why this is faster:**

* Without index: Check all N students
* With index: Check only k CS students (where k << N)
* Example: If 10,000 students, only 500 CS students → 20x speedup

Step 4: C++20 Ranges Integration
---------------------------------

Use C++20 ranges for post-query transformations and analysis.

Basic Ranges Example
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void ranges_basic_example(Table<Student>& students) {
       std::cout << "\n=== C++20 Ranges: Sorting by GPA ===\n";
       std::cout << std::string(60, '-') << "\n";

       // Get high GPA students using lambda
       auto high_gpa = students.find_if([](const Student& s) {
           return s.get_gpa() > 3.5;
       });

       std::cout << "Found " << high_gpa.size() << " students with GPA > 3.5\n\n";

       // Sort using C++20 ranges
       std::ranges::sort(high_gpa, [](const Student& a, const Student& b) {
           return a.get_gpa() > b.get_gpa();  // Descending order
       });

       std::cout << "Top 3 students by GPA:\n";
       for (size_t i = 0; i < std::min<size_t>(3, high_gpa.size()); ++i) {
           std::cout << "  " << (i + 1) << ". "
                     << std::setw(20) << std::left << high_gpa[i].get_name()
                     << " GPA: " << std::fixed << std::setprecision(2)
                     << high_gpa[i].get_gpa() << "\n";
       }
   }

Ranges for Transformation
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void ranges_transformation(Table<Student>& students) {
       std::cout << "\n=== C++20 Ranges: Functional Transformations ===\n";
       std::cout << std::string(60, '-') << "\n";

       // Get all students into a vector
       std::vector<Student> all_students;
       for (const auto& s : students) {
           all_students.push_back(s);
       }

       // Use ranges to extract names of Math students
       namespace views = std::ranges::views;

       auto math_student_names = all_students
           | views::filter([](const Student& s) {
               return s.get_department() == "Math";
             })
           | views::transform([](const Student& s) {
               return s.get_name();
             });

       std::cout << "Math students:\n";
       for (const auto& name : math_student_names) {
           std::cout << "  - " << name << "\n";
       }
   }

Step 5: Custom Predicate Functions
-----------------------------------

Create reusable predicate functions for common queries.

Predicate Function Pattern
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Reusable predicate builders
   auto in_department(const std::string& dept) {
       return [dept](const Student& s) {
           return s.get_department() == dept;
       };
   }

   auto min_gpa(double gpa) {
       return [gpa](const Student& s) {
           return s.get_gpa() >= gpa;
       };
   }

   auto max_age(int age) {
       return [age](const Student& s) {
           return s.get_age() <= age;
       };
   }

   // Combine predicates
   template<typename Pred1, typename Pred2>
   auto both(Pred1 p1, Pred2 p2) {
       return [p1, p2](const Student& s) {
           return p1(s) && p2(s);
       };
   }

   void predicate_composition(Table<Student>& students) {
       std::cout << "\n=== Composable Predicate Functions ===\n";
       std::cout << std::string(60, '-') << "\n";

       // Compose predicates
       auto elite_cs = both(
           in_department("CS"),
           min_gpa(3.7)
       );

       auto results = students.find_if(elite_cs);

       std::cout << "CS students with GPA ≥ 3.7:\n";
       for (const auto& student : results) {
           std::cout << "  - " << student.get_name() << "\n";
       }
   }

**Benefits:**

* Reusable query logic
* Testable predicate functions
* Clear, self-documenting code
* Easy to combine with ``&&``, ``||``, etc.

Step 6: Batch Processing Pattern
---------------------------------

Process large tables efficiently using batched iteration.

.. code-block:: cpp

   void batch_processing_pattern(Table<Student>& students) {
       std::cout << "\n=== Batch Processing Pattern ===\n";
       std::cout << std::string(60, '-') << "\n";

       std::cout << "Processing students in batches...\n";

       int batch_num = 0;
       int record_count = 0;
       const int BATCH_SIZE = 3;  // Small for demo

       // Iterate using the default iterator (batched internally)
       for (const auto& student : students) {
           if (record_count % BATCH_SIZE == 0) {
               batch_num++;
               std::cout << "\nBatch " << batch_num << ":\n";
           }

           std::cout << "  - " << student.get_name() << "\n";
           record_count++;
       }

       std::cout << "\n✓ Processed " << record_count << " records in "
                 << batch_num << " batches\n";
       std::cout << "Memory-efficient: Only small batches in memory at once!\n";
   }

**Why batching matters:**

* For 1M records: Loading all at once = ~24MB+ of RecordIds
* Batched loading: Constant memory usage regardless of table size
* Perfect for large-scale data processing

Step 7: Query Patterns and Best Practices
------------------------------------------

Pattern 1: Search and Update
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void search_and_update_pattern(Table<Student>& students) {
       std::cout << "\n=== Pattern: Search and Update ===\n";
       std::cout << std::string(60, '-') << "\n";

       // Find students who need GPA boost
       auto struggling_students = students.find_if([](const Student& s) {
           return s.get_gpa() < 3.5;
       });

       std::cout << "Found " << struggling_students.size()
                 << " students with GPA < 3.5\n";

       // In a real system, you might update records here
       std::cout << "Would apply academic support program to:\n";
       for (const auto& student : struggling_students) {
           std::cout << "  - " << student.get_name()
                     << " (GPA: " << std::fixed << std::setprecision(2)
                     << student.get_gpa() << ")\n";
       }
   }

Pattern 2: Count and Report
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void count_and_report_pattern(Table<Student>& students) {
       std::cout << "\n=== Pattern: Count and Report ===\n";
       std::cout << std::string(60, '-') << "\n";

       // Count students by category
       struct Stats {
           int total = 0;
           int high_gpa = 0;  // >= 3.7
           int young = 0;     // <= 20
           int elite = 0;     // high GPA + young
       };

       Stats stats;

       for (const auto& student : students) {
           stats.total++;

           if (student.get_gpa() >= 3.7) stats.high_gpa++;
           if (student.get_age() <= 20) stats.young++;
           if (student.get_gpa() >= 3.7 && student.get_age() <= 20)
               stats.elite++;
       }

       std::cout << "Student Statistics:\n";
       std::cout << "  Total students: " << stats.total << "\n";
       std::cout << "  High GPA (≥3.7): " << stats.high_gpa
                 << " (" << (stats.high_gpa * 100 / stats.total) << "%)\n";
       std::cout << "  Young (≤20): " << stats.young
                 << " (" << (stats.young * 100 / stats.total) << "%)\n";
       std::cout << "  Elite (both): " << stats.elite
                 << " (" << (stats.elite * 100 / stats.total) << "%)\n";
   }

Pattern 3: Top-N Query
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void top_n_pattern(Table<Student>& students, int n) {
       std::cout << "\n=== Pattern: Top-N Query ===\n";
       std::cout << std::string(60, '-') << "\n";

       // Get all students
       std::vector<Student> all_students;
       for (const auto& s : students) {
           all_students.push_back(s);
       }

       // Sort by GPA descending
       std::ranges::sort(all_students, [](const Student& a, const Student& b) {
           return a.get_gpa() > b.get_gpa();
       });

       std::cout << "Top " << n << " students by GPA:\n";
       for (int i = 0; i < std::min(n, static_cast<int>(all_students.size())); ++i) {
           std::cout << "  " << (i + 1) << ". "
                     << std::setw(20) << std::left << all_students[i].get_name()
                     << " GPA: " << std::fixed << std::setprecision(2)
                     << all_students[i].get_gpa() << "\n";
       }
   }

Step 8: Complete Advanced Query Program
----------------------------------------

.. code-block:: cpp

   int main() {
       try {
           Database db("advanced.db");

           // Create table with indexes
           auto& students = db.table<Student>("students")
               .add_index(Student::department, core::IndexType::MultiValue);

           // Populate if empty
           if (students.size() == 0) {
               populate_students(students);
           }

           std::cout << "Database contains " << students.size()
                     << " students\n\n";

           // Demonstrate advanced features
           basic_lambda_filtering(students);
           complex_criteria(students);
           optimized_two_stage_query(students);

           ranges_basic_example(students);
           ranges_transformation(students);

           predicate_composition(students);
           batch_processing_pattern(students);

           search_and_update_pattern(students);
           count_and_report_pattern(students);
           top_n_pattern(students, 5);

           return 0;
       } catch (const std::exception& e) {
           std::cerr << "Error: " << e.what() << std::endl;
           return 1;
       }
   }

Advanced Patterns Summary
-------------------------

Query Method Selection
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // 1. Index lookup - Fastest for equality queries
   auto student = table.find_by(Student::id, 1001);  // O(log n)

   // 2. Index + filter - Fast for filtered category queries
   auto cs = table.find_all_by(Student::dept, "CS"); // O(log n + k)
   // Then filter in memory

   // 3. Lambda predicate - Flexible for complex conditions
   auto results = table.find_if([](const Student& s) {
       return s.get_gpa() > 3.5 && s.get_age() < 21;
   });  // O(n) but flexible

   // 4. Full iteration - For aggregations and statistics
   for (const auto& record : table) {
       // Process each record
   }  // O(n) batched

Lambda Predicate Patterns
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Simple condition
   table.find_if([](const T& r) { return r.field == value; });

   // Multiple conditions (AND)
   table.find_if([](const T& r) {
       return r.field1 == val1 && r.field2 > val2;
   });

   // Multiple conditions (OR)
   table.find_if([](const T& r) {
       return r.field1 == val1 || r.field2 == val2;
   });

   // Complex logic
   table.find_if([](const T& r) {
       if (r.category == "A") return r.value > 100;
       else return r.value > 50;
   });

   // Capture external variables
   double threshold = 3.5;
   table.find_if([threshold](const T& r) {
       return r.score >= threshold;
   });

What You Learned
----------------

✅ Using ``find_if`` with lambda predicates for flexible queries

✅ Writing complex multi-criteria queries

✅ Optimizing queries by combining indexes with filters

✅ C++20 ranges integration for functional transformations

✅ Sorting results with ``std::ranges::sort``

✅ Creating reusable predicate functions

✅ Composing predicates for clean, maintainable code

✅ Batch processing pattern for memory efficiency

✅ Common query patterns (search-update, count-report, top-N)

✅ Best practices for query method selection

Exercises
---------

Try these advanced challenges:

1. **Query builder fluent API**

   Create a fluent API for building complex queries:

   .. code-block:: cpp

      auto results = QueryBuilder(students)
          .where_department("CS")
          .where_gpa_greater_than(3.5)
          .where_age_less_than(21)
          .execute();

2. **Cached query results**

   Implement a query cache that stores results of expensive queries:

   .. code-block:: cpp

      QueryCache cache;
      auto results = cache.get_or_compute(
          "cs_high_gpa",
          [&]() { return /* expensive query */; }
      );

3. **Query profiler**

   Build a profiler that measures query execution time and suggests optimizations.

4. **Pagination system**

   Implement pagination for large result sets:

   .. code-block:: cpp

      auto page = table.find_if(predicate)
          .skip(page_num * page_size)
          .take(page_size);

5. **Advanced reporting**

   Create a comprehensive reporting system that combines joins, aggregations, and custom calculations.

Congratulations!
----------------

You've completed all LearnQL tutorials! You now understand:

* Database fundamentals and CRUD operations
* Joins and relationships between tables
* Aggregations with ``count_by`` and ``average_by``
* Secondary indexes for performance optimization
* Advanced queries with lambda predicates
* C++20 ranges integration
* Best practices for production systems

Next Steps
----------

Explore more LearnQL features:

* :doc:`/api/core` - Complete API reference
* :doc:`/architecture/overview` - Deep dive into internals
* :doc:`/resources/examples` - More example projects

.. tip::
   LearnQL is educational. For production systems, consider mature databases like PostgreSQL, SQLite, or RocksDB. However, the patterns and techniques you learned here transfer directly to any database system!

   Key takeaways to remember:

   * Always use indexes for frequently queried fields
   * Combine index lookups with in-memory filtering for optimal performance
   * Choose the right query method based on your use case
   * Measure performance before and after optimizations
   * Write reusable, testable query logic

Thank you for learning with LearnQL!
