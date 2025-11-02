Ranges Module
=============

.. contents:: Table of Contents
   :local:
   :depth: 2

Overview
--------

The Ranges module integrates LearnQL with C++20 ranges, enabling modern, composable query pipelines. It provides custom range adaptors and views that work seamlessly with standard library ranges, allowing you to build complex data transformations using the pipe (``|``) operator.

**Key Features:**

- Full C++20 ranges compatibility
- Custom range adaptors (``order_by``, ``select``, ``limit``)
- Lazy evaluation for memory efficiency
- Composable transformations with ``|`` operator
- Integration with standard range adaptors (``std::views::filter``, ``std::views::transform``, etc.)

**Module Components:**

- ``QueryView<T>`` - Range view wrapper for query results
- ``ProxyVector<T>`` - Lazy-loading result container
- Custom adaptors: ``order_by``, ``select``, ``limit``
- Integration with ``std::ranges`` and ``std::views``

Quick Start
-----------

Basic Ranges Usage
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <learnql/LearnQL.hpp>
   #include <ranges>

   Database db("school.db");
   auto& students = db.table<Student>("students");

   // Use standard range adaptors
   auto names = students.all()
       | std::views::transform([](auto& s) { return s.get_name(); })
       | std::views::take(10);

   for (const auto& name : names) {
       std::cout << name << "\n";
   }

Custom Range Adaptors
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <learnql/ranges/Adaptors.hpp>

   // Order by a field
   auto sorted = students.view()
       | order_by(&Student::get_gpa, false)  // Descending
       | std::views::take(5);

   // Select specific field
   auto ages = students.view()
       | select(&Student::get_age);

Composing Pipelines
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   auto top_performers = students.view()
       | std::views::filter([](auto& s) { return s.get_gpa() >= 3.5; })
       | order_by(&Student::get_gpa, false)
       | select(&Student::get_name)
       | std::views::take(10);

   for (const auto& name : top_performers) {
       std::cout << "Top student: " << name << "\n";
   }

Core Classes
------------

QueryView Class
~~~~~~~~~~~~~~~

.. doxygenclass:: learnql::ranges::QueryView
   :members:

A range view that wraps query results and integrates with C++20 ranges.

**Template Parameters:**

- ``T`` - Element type

**Constructors:**

.. code-block:: cpp

   // From vector
   explicit QueryView(std::vector<T> data);

   // From ProxyVector (lazy result container)
   template<std::size_t BatchSize>
   explicit QueryView(ProxyVector<T, BatchSize> proxy_data);

**Methods:**

``begin()`` / ``end()`` - Iteration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   Iterator begin();
   Iterator end();

Returns iterators for range-based for loops and algorithms.

**Example:**

.. code-block:: cpp

   QueryView view(students.all());

   for (const auto& student : view) {
       std::cout << student.get_name() << "\n";
   }

``size()`` - Get Element Count
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   [[nodiscard]] std::size_t size() const noexcept;

Returns the number of elements in the view.

``empty()`` - Check if Empty
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   [[nodiscard]] bool empty() const noexcept;

Returns ``true`` if view contains no elements.

``to_vector()`` - Materialize to Vector
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   [[nodiscard]] std::vector<T> to_vector() const;

Materializes the view into a ``std::vector``.

**Example:**

.. code-block:: cpp

   QueryView view(students.all());
   std::vector<Student> materialized = view.to_vector();

ProxyVector Class
~~~~~~~~~~~~~~~~~

.. doxygenclass:: learnql::ranges::ProxyVector
   :members:

A lazy-loading container that loads records in batches as you iterate.

**Template Parameters:**

- ``T`` - Element type
- ``BatchSize`` - Number of records per batch (default: 10)

**Key Characteristics:**

- Lazy evaluation - data loaded on demand
- Memory efficient - only keeps current batch in memory
- Forward iteration - can iterate once
- Range-compatible - works with C++20 ranges

**Example:**

.. code-block:: cpp

   // Get all students (lazy)
   ProxyVector<Student> students_proxy = students.all();

   // Only loads batches as you iterate
   for (const auto& student : students_proxy) {
       // Batch of 10 loaded automatically
       process(student);
   }

``materialize()`` - Load All Data
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   [[nodiscard]] std::vector<T> materialize() const;

Loads all records into memory at once.

**Example:**

.. code-block:: cpp

   std::vector<Student> all_students = students.all().materialize();

**Warning:** Avoid for large result sets - defeats lazy evaluation!

Custom Range Adaptors
---------------------

order_by Adaptor
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   template<typename Proj>
   auto order_by(Proj proj, bool ascending = true);

Sorts a range by a projection (e.g., member function pointer).

**Parameters:**

- ``proj`` - Projection function (typically a getter)
- ``ascending`` - Sort order (``true`` = ascending, ``false`` = descending)

**Returns:** Sorted vector

**Example:**

.. code-block:: cpp

   // Sort by GPA ascending
   auto sorted_asc = students.view() | order_by(&Student::get_gpa);

   // Sort by GPA descending
   auto sorted_desc = students.view() | order_by(&Student::get_gpa, false);

   // Sort by name
   auto by_name = students.view() | order_by(&Student::get_name);

select Adaptor
~~~~~~~~~~~~~~

.. code-block:: cpp

   template<typename Proj>
   auto select(Proj proj);

Projects each element using a function (similar to SQL SELECT).

**Parameters:**

- ``proj`` - Projection function

**Returns:** Transform view

**Example:**

.. code-block:: cpp

   // Get all student names
   auto names = students.view() | select(&Student::get_name);

   // Get all GPAs
   auto gpas = students.view() | select(&Student::get_gpa);

   // Custom projection
   auto name_gpa = students.view()
       | select([](auto& s) {
           return s.get_name() + " (" + std::to_string(s.get_gpa()) + ")";
         });

limit Adaptor
~~~~~~~~~~~~~

.. code-block:: cpp

   auto limit(std::size_t count);

Limits the number of elements (wrapper around ``std::views::take``).

**Parameters:**

- ``count`` - Maximum number of elements

**Returns:** Limited view

**Example:**

.. code-block:: cpp

   // Get first 10 students
   auto top_10 = students.view() | limit(10);

   // Combined with filtering
   auto first_5_adults = students.view()
       | std::views::filter([](auto& s) { return s.get_age() >= 18; })
       | limit(5);

Standard Range Adaptors
-----------------------

LearnQL works with all standard C++20 range adaptors:

filter
~~~~~~

.. code-block:: cpp

   #include <ranges>

   // Filter students by age
   auto adults = students.view()
       | std::views::filter([](auto& s) { return s.get_age() >= 18; });

   // Filter by GPA
   auto honor_roll = students.view()
       | std::views::filter([](auto& s) { return s.get_gpa() >= 3.5; });

   // Multiple filters
   auto filtered = students.view()
       | std::views::filter([](auto& s) { return s.get_age() >= 18; })
       | std::views::filter([](auto& s) { return s.get_gpa() >= 3.0; });

transform
~~~~~~~~~

.. code-block:: cpp

   // Transform to names
   auto names = students.view()
       | std::views::transform([](auto& s) { return s.get_name(); });

   // Transform to custom type
   auto summaries = students.view()
       | std::views::transform([](auto& s) {
           return StudentSummary{s.get_id(), s.get_name(), s.get_gpa()};
         });

take / take_while
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Take first 10
   auto first_10 = students.view() | std::views::take(10);

   // Take while condition is true
   auto while_high_gpa = students.view()
       | order_by(&Student::get_gpa, false)
       | std::views::take_while([](auto& s) { return s.get_gpa() >= 3.5; });

drop / drop_while
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Skip first 10
   auto skip_10 = students.view() | std::views::drop(10);

   // Drop while condition is true
   auto after_low_gpa = students.view()
       | order_by(&Student::get_gpa)
       | std::views::drop_while([](auto& s) { return s.get_gpa() < 2.0; });

reverse
~~~~~~~

.. code-block:: cpp

   // Reverse order
   auto reversed = students.view()
       | order_by(&Student::get_id)
       | std::views::reverse;

Usage Examples
--------------

Simple Filtering and Projection
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Get names of adult students
   auto adult_names = students.view()
       | std::views::filter([](auto& s) { return s.get_age() >= 18; })
       | std::views::transform([](auto& s) { return s.get_name(); });

   for (const auto& name : adult_names) {
       std::cout << name << "\n";
   }

Top N Query
~~~~~~~~~~~

.. code-block:: cpp

   // Get top 5 students by GPA
   auto top_5 = students.view()
       | order_by(&Student::get_gpa, false)
       | std::views::take(5);

   std::cout << "Top 5 Students:\n";
   for (const auto& student : top_5) {
       std::cout << student.get_name() << ": " << student.get_gpa() << "\n";
   }

Pagination
~~~~~~~~~~

.. code-block:: cpp

   std::size_t page_size = 20;
   std::size_t page_num = 2;  // 0-indexed

   auto page = students.view()
       | order_by(&Student::get_id)
       | std::views::drop(page_num * page_size)
       | std::views::take(page_size);

   for (const auto& student : page) {
       std::cout << student.get_id() << ": " << student.get_name() << "\n";
   }

Complex Pipeline
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Find honor students, sort by GPA, get names and GPAs
   auto honor_summary = students.view()
       | std::views::filter([](auto& s) {
           return s.get_gpa() >= 3.5 && s.get_age() >= 18;
         })
       | order_by(&Student::get_gpa, false)
       | std::views::transform([](auto& s) {
           return std::make_pair(s.get_name(), s.get_gpa());
         })
       | std::views::take(10);

   std::cout << "Top 10 Honor Students:\n";
   for (const auto& [name, gpa] : honor_summary) {
       std::cout << name << ": " << gpa << "\n";
   }

Grouping (Manual)
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <map>

   // Group students by major
   std::map<std::string, std::vector<Student>> by_major;

   for (auto& student : students.all()) {
       by_major[student.get_major()].push_back(student);
   }

   // Process each group
   for (const auto& [major, students_in_major] : by_major) {
       std::cout << major << ": " << students_in_major.size() << " students\n";
   }

Aggregation
~~~~~~~~~~~

.. code-block:: cpp

   #include <numeric>

   // Calculate average GPA
   auto gpas = students.view()
       | std::views::transform([](auto& s) { return s.get_gpa(); });

   auto gpa_vec = gpas | std::ranges::to<std::vector>();
   double avg_gpa = std::accumulate(gpa_vec.begin(), gpa_vec.end(), 0.0)
                    / gpa_vec.size();

   std::cout << "Average GPA: " << avg_gpa << "\n";

   // Count students by condition
   auto adult_count = std::ranges::count_if(
       students.all(),
       [](auto& s) { return s.get_age() >= 18; }
   );

   std::cout << "Adult students: " << adult_count << "\n";

Advanced Patterns
-----------------

Lazy Evaluation Chain
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Everything is lazy until materialized
   auto lazy_pipeline = students.all()  // ProxyVector (lazy)
       | std::views::filter([](auto& s) { return s.get_gpa() > 3.0; })
       | std::views::transform([](auto& s) { return s.get_name(); })
       | std::views::take(100);

   // No database reads yet!

   // Iteration triggers loading
   for (const auto& name : lazy_pipeline) {
       std::cout << name << "\n";  // ← Batches loaded on demand
   }

Combining Multiple Tables
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   auto& students = db.table<Student>("students");
   auto& courses = db.table<Course>("courses");

   // Manual join (cartesian product + filter)
   auto student_vec = students.all().materialize();
   auto course_vec = courses.all().materialize();

   auto enrolled = student_vec
       | std::views::transform([&](auto& student) {
           return course_vec
               | std::views::filter([&](auto& course) {
                   return is_enrolled(student.get_id(), course.get_id());
                 })
               | std::views::transform([&](auto& course) {
                   return std::make_pair(student.get_name(), course.get_name());
                 });
         })
       | std::views::join;  // Flatten

   for (const auto& [student_name, course_name] : enrolled) {
       std::cout << student_name << " -> " << course_name << "\n";
   }

Custom View Adaptor
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Custom adaptor: chunk (split into groups)
   template<std::ranges::input_range R>
   auto chunk(R&& range, std::size_t chunk_size) {
       return std::forward<R>(range)
           | std::views::transform([chunk_size, i = 0](auto& elem) mutable {
               return std::make_pair(i++ / chunk_size, elem);
             });
   }

   // Usage
   auto chunked = students.view()
       | chunk(5)
       | std::views::transform([](auto pair) { return pair.second; });

Performance Considerations
--------------------------

Lazy vs. Eager Evaluation
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // LAZY - memory efficient, loads on demand
   auto lazy = students.all()  // ProxyVector
       | std::views::filter([](auto& s) { return s.get_age() >= 18; });

   // EAGER - loads all into memory
   auto eager = students.all().materialize();
   auto filtered = eager
       | std::views::filter([](auto& s) { return s.get_age() >= 18; })
       | std::ranges::to<std::vector>();

**Guideline:** Use lazy for large datasets, eager when you need multiple passes.

Materialization Points
~~~~~~~~~~~~~~~~~~~~~~

Some adaptors require materialization:

.. code-block:: cpp

   // Requires materialization (sorting)
   auto sorted = students.view()
       | order_by(&Student::get_gpa);  // ← Materializes here

   // Doesn't require materialization (lazy)
   auto filtered = students.all()
       | std::views::filter([](auto& s) { return s.get_age() >= 18; });

Range Overhead
~~~~~~~~~~~~~~

Range adaptors have minimal overhead:

.. code-block:: cpp

   // Nearly zero overhead
   auto pipeline = students.all()
       | std::views::filter([](auto& s) { return s.get_age() >= 18; })
       | std::views::transform([](auto& s) { return s.get_name(); });

   // Equivalent to hand-written loop:
   for (auto& student : students.all()) {
       if (student.get_age() >= 18) {
           auto name = student.get_name();
           // process(name);
       }
   }

Best Practices
--------------

1. **Prefer Lazy Evaluation**

   .. code-block:: cpp

      // GOOD - lazy, memory efficient
      auto results = students.all()
          | std::views::filter([](auto& s) { return s.get_gpa() > 3.0; });

      // AVOID - eager, uses more memory
      auto all = students.all().materialize();
      auto results = all
          | std::views::filter([](auto& s) { return s.get_gpa() > 3.0; })
          | std::ranges::to<std::vector>();

2. **Use order_by Sparingly**

   .. code-block:: cpp

      // Sorting is expensive - only do it when necessary
      auto sorted = students.view()
          | order_by(&Student::get_gpa, false)
          | std::views::take(10);  // At least we limit the result

3. **Combine Filters Early**

   .. code-block:: cpp

      // GOOD - reduce dataset early
      auto results = students.all()
          | std::views::filter([](auto& s) {
              return s.get_age() >= 18 && s.get_gpa() >= 3.0;
            });

      // LESS EFFICIENT - two passes
      auto results = students.all()
          | std::views::filter([](auto& s) { return s.get_age() >= 18; })
          | std::views::filter([](auto& s) { return s.get_gpa() >= 3.0; });

4. **Limit Result Sets**

   .. code-block:: cpp

      // Always use take() when you don't need all results
      auto top_10 = students.view()
          | order_by(&Student::get_gpa, false)
          | std::views::take(10);

5. **Avoid Multiple Materializations**

   .. code-block:: cpp

      // WRONG - materializes twice
      auto names1 = students.all().materialize()
          | std::views::transform([](auto& s) { return s.get_name(); })
          | std::ranges::to<std::vector>();

      auto names2 = students.all().materialize()
          | std::views::transform([](auto& s) { return s.get_name(); })
          | std::ranges::to<std::vector>();

      // RIGHT - materialize once
      auto all_students = students.all().materialize();

      auto names1 = all_students
          | std::views::transform([](auto& s) { return s.get_name(); })
          | std::ranges::to<std::vector>();

      auto names2 = all_students
          | std::views::transform([](auto& s) { return s.get_name(); })
          | std::ranges::to<std::vector>();

Troubleshooting
---------------

Dangling References
~~~~~~~~~~~~~~~~~~~

**Problem:** Crash or undefined behavior with range pipelines

**Cause:** Range views don't own data - temporary objects destroyed

**Solution:** Materialize when necessary

.. code-block:: cpp

   // WRONG - dangling reference
   auto get_names() {
       auto students_vec = students.all().materialize();
       return students_vec | std::views::transform([](auto& s) {
           return s.get_name();
       });
       // students_vec destroyed here!
   }

   // RIGHT - materialize result
   auto get_names() {
       auto students_vec = students.all().materialize();
       return students_vec
           | std::views::transform([](auto& s) { return s.get_name(); })
           | std::ranges::to<std::vector>();  // ← Own the data
   }

Iterator Invalidation
~~~~~~~~~~~~~~~~~~~~~

**Problem:** ``ProxyVector`` can only be iterated once

**Cause:** Lazy loading - batches discarded after iteration

**Solution:** Materialize if you need multiple passes

.. code-block:: cpp

   auto results = students.all();  // ProxyVector

   // First iteration - OK
   for (auto& s : results) { /* ... */ }

   // Second iteration - ERROR (already consumed)
   for (auto& s : results) { /* ... */ }

   // Solution: materialize
   auto materialized = students.all().materialize();
   for (auto& s : materialized) { /* ... */ }
   for (auto& s : materialized) { /* ... */ }  // OK

See Also
--------

- :doc:`query` - Query module that produces ranges
- :doc:`coroutines` - Generator for lazy sequences
- :doc:`../tutorials/ranges` - Ranges tutorial
- :doc:`../guides/performance` - Performance optimization

**Related Headers:**

- ``learnql/ranges/QueryView.hpp``
- ``learnql/ranges/ProxyVector.hpp``
- ``learnql/ranges/Adaptors.hpp``

**External Resources:**

- `C++20 Ranges <https://en.cppreference.com/w/cpp/ranges>`_
- `std::views <https://en.cppreference.com/w/cpp/ranges#Range_adaptors>`_
