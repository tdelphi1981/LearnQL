Debug Module
============

.. contents:: Table of Contents
   :local:
   :depth: 2

Overview
--------

The Debug module provides comprehensive profiling, statistics collection, and query execution visualization tools for LearnQL. It helps you understand query performance, diagnose bottlenecks, and optimize database operations through detailed metrics and execution plans.

**Key Features:**

- Query performance profiling with microsecond precision
- Statistics collection for tables, indexes, and queries
- Execution plan visualization
- RAII-based scoped profiling
- Aggregate performance reports
- Database inspection utilities

**Module Components:**

- ``Profiler`` - Singleton profiler for collecting metrics
- ``Timer`` - RAII timer for measuring code blocks
- ``ScopedProfiler`` - Automatic metric recording
- ``StatisticsCollector`` - Database statistics
- ``ExecutionPlan`` - Query execution plan visualization
- ``DbInspector`` - Database inspection utility

Quick Start
-----------

Basic Profiling
~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <learnql/debug/Profiler.hpp>

   // Profile a code block
   {
       Timer timer("Query execution");
       auto results = students.where(Student::age >= 18);
       // Automatically prints: [TIMER] Query execution: 2.345 ms
   }

   // Or use scoped profiler for automatic recording
   {
       ScopedProfiler prof("Table scan");
       for (auto& student : students.all()) {
           process(student);
       }
       prof.set_rows_processed(students.size());
   }

   // Print summary
   Profiler::instance().print_summary();

Collecting Statistics
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <learnql/debug/Statistics.hpp>

   auto& stats = StatisticsCollector::instance();

   // Record table statistics
   TableStatistics table_stats{
       .table_name = "students",
       .row_count = 1500,
       .page_count = 50,
       .total_bytes = 204800
   };
   stats.record_table(table_stats);

   // Print all statistics
   stats.print_all();

Execution Plans
~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <learnql/debug/ExecutionPlan.hpp>

   // Build execution plan
   ExecutionPlanBuilder builder("SELECT * FROM students WHERE age >= 18");

   auto root = builder.table_scan("students", 1500);
   auto filter = builder.filter("age >= 18", 800);
   filter->add_child(root);

   builder.set_root(filter);
   auto plan = builder.build();

   plan.print();

Class Reference
---------------

Profiler Class
~~~~~~~~~~~~~~

.. doxygenclass:: learnql::debug::Profiler
   :members:

A singleton profiler that collects and aggregates performance metrics.

**Access:**

.. code-block:: cpp

   static Profiler& instance();

Returns the singleton instance.

Core Methods
^^^^^^^^^^^^

``record()`` - Record Performance Metric
"""""""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   void record(const PerformanceMetrics& metrics);

Records a performance measurement.

**Parameters:**

- ``metrics`` - Performance metrics to record

**Example:**

.. code-block:: cpp

   PerformanceMetrics metrics{
       .operation_name = "Table scan",
       .duration = std::chrono::microseconds{5000},
       .rows_processed = 1000,
       .memory_used = 40960
   };

   Profiler::instance().record(metrics);

``print_summary()`` - Print Performance Report
"""""""""""""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   void print_summary() const;

Prints a comprehensive performance summary with per-operation breakdowns.

**Example Output:**

.. code-block:: text

   === Performance Summary ===

   Total operations: 15
   Total duration: 125.450 ms
   Total rows processed: 12500
   Overall throughput: 99600.80 rows/sec

   Per-operation breakdown:
   Operation                     Count    Total (ms)    Avg (ms)    Rows      Rows/sec
   ──────────────────────────────────────────────────────────────────────────────────
   Table scan                    5        45.200        9.040       5000      110619.47
   Index lookup                  8        15.300        1.912       3500      228758.17
   Filter operation              2        64.950        32.475      4000      61574.67

``clear()`` - Clear Metrics
""""""""""""""""""""""""""""

.. code-block:: cpp

   void clear();

Clears all recorded metrics.

``metrics()`` - Get All Metrics
""""""""""""""""""""""""""""""""

.. code-block:: cpp

   [[nodiscard]] const std::vector<PerformanceMetrics>& metrics() const noexcept;

Returns all recorded metrics.

PerformanceMetrics Structure
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   struct PerformanceMetrics {
       std::string operation_name;
       std::chrono::microseconds duration;
       std::size_t rows_processed;
       std::size_t memory_used;

       [[nodiscard]] double duration_ms() const;
       [[nodiscard]] double rows_per_second() const;
   };

Timer Class
~~~~~~~~~~~

.. doxygenclass:: learnql::debug::Timer
   :members:

RAII timer that automatically prints elapsed time when destroyed.

**Constructor:**

.. code-block:: cpp

   explicit Timer(std::string name);

**Example:**

.. code-block:: cpp

   void expensive_operation() {
       Timer timer("Expensive operation");
       // ... code to profile ...
   }  // Prints: [TIMER] Expensive operation: 123.456 ms

Methods
^^^^^^^

``stop()`` - Stop Timer
"""""""""""""""""""""""

.. code-block:: cpp

   std::chrono::microseconds stop();

Stops the timer and returns elapsed time.

**Example:**

.. code-block:: cpp

   Timer timer("Operation");
   // ... code ...
   auto elapsed = timer.stop();
   std::cout << "Took " << elapsed.count() << " microseconds\n";

``print()`` - Print Elapsed Time
"""""""""""""""""""""""""""""""""

.. code-block:: cpp

   void print() const;

Prints the elapsed time (automatically called by destructor).

ScopedProfiler Class
~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: learnql::debug::ScopedProfiler
   :members:

RAII profiler that automatically records metrics when destroyed.

**Constructor:**

.. code-block:: cpp

   explicit ScopedProfiler(std::string operation_name);

**Example:**

.. code-block:: cpp

   {
       ScopedProfiler prof("Query processing");

       auto results = students.where(Student::age >= 18);
       prof.set_rows_processed(results.size());

       for (auto& student : results) {
           process(student);
       }

       prof.set_memory_used(results.size() * sizeof(Student));
   }  // Metrics automatically recorded here

Methods
^^^^^^^

``set_rows_processed()``
"""""""""""""""""""""""""

.. code-block:: cpp

   void set_rows_processed(std::size_t rows);

Sets the number of rows processed by the operation.

``set_memory_used()``
"""""""""""""""""""""

.. code-block:: cpp

   void set_memory_used(std::size_t bytes);

Sets the memory used by the operation.

StatisticsCollector Class
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: learnql::debug::StatisticsCollector
   :members:

Singleton collector for database statistics.

**Access:**

.. code-block:: cpp

   static StatisticsCollector& instance();

Core Methods
^^^^^^^^^^^^

``record_table()`` - Record Table Statistics
"""""""""""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   void record_table(const TableStatistics& stats);

Records statistics for a table.

``record_index()`` - Record Index Statistics
"""""""""""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   void record_index(const IndexStatistics& stats);

Records statistics for an index.

``record_query()`` - Record Query Execution
""""""""""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   void record_query(
       const std::string& query_desc,
       std::chrono::microseconds duration,
       std::size_t rows_returned
   );

Records a query execution.

``print_all()`` - Print All Statistics
"""""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   void print_all() const;

Prints all collected statistics.

**Example Output:**

.. code-block:: text

   === Database Statistics ===

   --- Table Statistics ---
   Table: students
     Rows: 1500
     Pages: 50
     Total size: 204800 bytes
     Avg row size: 136.53 bytes
     Avg rows per page: 30.00
     Indexes: 3

   --- Index Statistics ---
   Index: students_age_idx
     Table: students
     Field: age
     Entries: 1500
     Unique values: 45
     Selectivity: 0.0300
     Total size: 24576 bytes

   --- Query Statistics ---
   Query: SELECT * WHERE age >= 18
     Executions: 150
     Avg duration: 2.345 ms
     Avg rows: 1200.00

Statistics Structures
^^^^^^^^^^^^^^^^^^^^^

**TableStatistics:**

.. code-block:: cpp

   struct TableStatistics {
       std::string table_name;
       std::size_t row_count;
       std::size_t page_count;
       std::size_t total_bytes;
       std::size_t index_count;

       [[nodiscard]] double avg_row_size() const;
       [[nodiscard]] double avg_rows_per_page() const;
       void print() const;
   };

**IndexStatistics:**

.. code-block:: cpp

   struct IndexStatistics {
       std::string index_name;
       std::string table_name;
       std::string indexed_field;
       std::size_t entry_count;
       std::size_t unique_values;
       std::size_t total_bytes;

       [[nodiscard]] double selectivity() const;
       void print() const;
   };

**QueryStatistics:**

.. code-block:: cpp

   struct QueryStatistics {
       std::string query_description;
       std::size_t execution_count;
       std::chrono::microseconds total_duration;
       std::size_t total_rows_returned;

       [[nodiscard]] double avg_duration_ms() const;
       [[nodiscard]] double avg_rows_returned() const;
       void print() const;
   };

ExecutionPlan Class
~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: learnql::debug::ExecutionPlan
   :members:

Visualizes query execution plans as a tree structure.

**Constructor:**

.. code-block:: cpp

   explicit ExecutionPlan(std::string query_text);

Methods
^^^^^^^

``set_root()`` - Set Root Node
"""""""""""""""""""""""""""""""

.. code-block:: cpp

   void set_root(std::shared_ptr<ExecutionPlanNode> root);

Sets the root node of the execution plan tree.

``print()`` - Print Plan
"""""""""""""""""""""""""

.. code-block:: cpp

   void print() const;

Prints the execution plan as a tree.

**Example Output:**

.. code-block:: text

   === Query Execution Plan ===
   Query: SELECT * FROM students WHERE age >= 18

   └── FILTER: Condition: age >= 18 (est. rows: 800, cost: 400.00)
       └── TABLE_SCAN: Table: students (est. rows: 1500, cost: 1500.00)

ExecutionPlanNode Class
~~~~~~~~~~~~~~~~~~~~~~~

Represents a node in the execution plan tree.

**Constructor:**

.. code-block:: cpp

   ExecutionPlanNode(OperationType type, std::string description);

**Methods:**

.. code-block:: cpp

   void set_estimated_rows(std::size_t rows);
   void set_estimated_cost(double cost);
   void add_child(std::shared_ptr<ExecutionPlanNode> child);

ExecutionPlanBuilder Class
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: learnql::debug::ExecutionPlanBuilder
   :members:

Builder for creating execution plans.

**Constructor:**

.. code-block:: cpp

   explicit ExecutionPlanBuilder(std::string query_text);

Factory Methods
^^^^^^^^^^^^^^^

.. code-block:: cpp

   static std::shared_ptr<ExecutionPlanNode> table_scan(
       const std::string& table_name,
       std::size_t estimated_rows = 0
   );

   static std::shared_ptr<ExecutionPlanNode> index_scan(
       const std::string& index_name,
       std::size_t estimated_rows = 0
   );

   static std::shared_ptr<ExecutionPlanNode> filter(
       const std::string& condition,
       std::size_t estimated_rows = 0
   );

   static std::shared_ptr<ExecutionPlanNode> join(
       const std::string& join_type,
       const std::string& condition,
       std::size_t estimated_rows = 0
   );

   static std::shared_ptr<ExecutionPlanNode> sort(
       const std::string& fields,
       std::size_t estimated_rows = 0
   );

   static std::shared_ptr<ExecutionPlanNode> limit(
       std::size_t limit_count
   );

Usage Examples
--------------

Profiling Queries
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <learnql/debug/Profiler.hpp>

   // Clear previous metrics
   Profiler::instance().clear();

   // Profile multiple operations
   {
       Timer t1("Load all students");
       auto all_students = students.all().materialize();
   }

   {
       Timer t2("Filter by age");
       auto adults = students.where(Student::age >= 18);
   }

   {
       Timer t3("Index lookup");
       auto student = students.find(12345);
   }

   // Print comprehensive report
   Profiler::instance().print_summary();

Automatic Metric Collection
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void process_students(Table<Student>& students) {
       ScopedProfiler prof("Student processing");

       auto results = students.where(Student::gpa >= 3.0);
       std::size_t count = 0;

       for (auto& student : results) {
           // Process student
           ++count;
       }

       prof.set_rows_processed(count);
       prof.set_memory_used(count * sizeof(Student));

       // Metrics automatically recorded when prof goes out of scope
   }

   // Later, view all metrics
   for (const auto& metric : Profiler::instance().metrics()) {
       std::cout << metric.operation_name << ": "
                 << metric.duration_ms() << " ms\n";
   }

Collecting Table Statistics
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void collect_table_stats(Database& db) {
       auto& stats = StatisticsCollector::instance();
       auto& catalog = db.metadata();

       // Collect stats for all tables
       for (const auto& table_meta : catalog.tables().all()) {
           if (table_meta.is_system_table) continue;

           TableStatistics ts{
               .table_name = table_meta.table_name,
               .row_count = table_meta.record_count,
               .page_count = estimate_pages(table_meta),
               .total_bytes = table_meta.record_count * avg_row_size,
               .index_count = count_indexes(table_meta.table_name, catalog)
           };

           stats.record_table(ts);
       }

       stats.print_all();
   }

Building Execution Plans
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void visualize_query_plan() {
       ExecutionPlanBuilder builder(
           "SELECT * FROM students WHERE age >= 18 ORDER BY gpa DESC LIMIT 10"
       );

       // Build plan tree (bottom-up)
       auto scan = builder.table_scan("students", 1500);
       auto filter = builder.filter("age >= 18", 800);
       auto sort = builder.sort("gpa DESC", 800);
       auto limit = builder.limit(10);

       // Connect nodes
       filter->add_child(scan);
       sort->add_child(filter);
       limit->add_child(sort);

       // Set root and build
       builder.set_root(limit);
       auto plan = builder.build();

       plan.print();
   }

   // Output:
   // === Query Execution Plan ===
   // Query: SELECT * FROM students WHERE age >= 18 ORDER BY gpa DESC LIMIT 10
   //
   // └── LIMIT: Limit: 10 (est. rows: 10, cost: 1.00)
   //     └── SORT: Fields: gpa DESC (est. rows: 800, cost: 9287.71)
   //         └── FILTER: Condition: age >= 18 (est. rows: 800, cost: 400.00)
   //             └── TABLE_SCAN: Table: students (est. rows: 1500, cost: 1500.00)

Comparing Query Performance
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void compare_queries() {
       Profiler::instance().clear();

       // Query 1: Full table scan
       {
           ScopedProfiler prof("Full table scan");
           auto results = students.all();
           std::size_t count = 0;
           for (auto& s : results) {
               if (s.get_age() >= 18) ++count;
           }
           prof.set_rows_processed(count);
       }

       // Query 2: With filter expression
       {
           ScopedProfiler prof("Expression filter");
           auto results = students.where(Student::age >= 18);
           prof.set_rows_processed(results.size());
       }

       // Query 3: With index (if available)
       {
           ScopedProfiler prof("Index lookup");
           // Assuming age index exists
           auto results = students.where(Student::age >= 18);
           prof.set_rows_processed(results.size());
       }

       Profiler::instance().print_summary();
   }

Performance Benchmarking
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void benchmark_operations(std::size_t iterations) {
       Profiler::instance().clear();

       for (std::size_t i = 0; i < iterations; ++i) {
           {
               ScopedProfiler prof("Insert");
               Student s{static_cast<int>(i), "Test", 3.5};
               students.insert(s);
               prof.set_rows_processed(1);
           }

           {
               ScopedProfiler prof("Find");
               auto result = students.find(static_cast<int>(i));
               prof.set_rows_processed(result ? 1 : 0);
           }

           {
               ScopedProfiler prof("Update");
               auto s = students.find(static_cast<int>(i));
               if (s) {
                   s->set_gpa(3.6);
                   students.update(*s);
                   prof.set_rows_processed(1);
               }
           }
       }

       std::cout << "\n=== Benchmark Results (" << iterations << " iterations) ===\n";
       Profiler::instance().print_summary();
   }

Best Practices
--------------

1. **Clear Metrics Before Benchmarking**

   .. code-block:: cpp

      // Always start fresh
      Profiler::instance().clear();
      StatisticsCollector::instance().clear();

      // Run benchmarks
      benchmark();

2. **Use Scoped Profilers**

   .. code-block:: cpp

      // GOOD - automatic cleanup
      {
          ScopedProfiler prof("Operation");
          do_work();
      }  // Metrics recorded automatically

      // AVOID - manual timing
      auto start = std::chrono::high_resolution_clock::now();
      do_work();
      auto end = std::chrono::high_resolution_clock::now();
      // ... manual metric recording ...

3. **Profile Representative Workloads**

   .. code-block:: cpp

      // Don't profile tiny datasets
      // Profile with realistic data sizes
      for (int i = 0; i < 10000; ++i) {
          students.insert(generate_student());
      }

      // Now profile
      benchmark_queries();

4. **Separate Warmup from Measurement**

   .. code-block:: cpp

      // Warmup (cache loading, etc.)
      for (int i = 0; i < 10; ++i) {
          students.where(Student::age >= 18);
      }

      // Clear warmup metrics
      Profiler::instance().clear();

      // Actual measurement
      for (int i = 0; i < 100; ++i) {
          ScopedProfiler prof("Query");
          students.where(Student::age >= 18);
      }

5. **Log Statistics Periodically**

   .. code-block:: cpp

      void log_stats_periodically() {
          static auto last_log = std::chrono::steady_clock::now();
          auto now = std::chrono::steady_clock::now();

          if (now - last_log > std::chrono::minutes{5}) {
              std::cout << "\n=== Periodic Statistics ===\n";
              StatisticsCollector::instance().print_all();
              last_log = now;
          }
       }

Integration with Database Inspector
------------------------------------

.. code-block:: cpp

   #include <learnql/utils/DbInspector.hpp>

   Database db("school.db");
   utils::DbInspector inspector(db);

   // Comprehensive database inspection
   inspector.print_summary();       // Overview
   inspector.print_tables();        // Table details
   inspector.print_indexes();       // Index details
   inspector.print_statistics();    // Collected statistics

   // Export to file
   std::ofstream report("db_report.txt");
   inspector.print_summary(report);
   inspector.print_tables(report);

Troubleshooting
---------------

Missing Metrics
~~~~~~~~~~~~~~~

**Problem:** ``print_summary()`` shows no data

**Cause:** Metrics not recorded or cleared

**Solution:** Ensure profilers are used and not cleared prematurely

.. code-block:: cpp

   // WRONG - cleared too early
   Profiler::instance().clear();
   {
       ScopedProfiler prof("Op");
       work();
   }
   Profiler::instance().clear();  // ← Cleared the metric!
   Profiler::instance().print_summary();  // Empty

   // RIGHT
   Profiler::instance().clear();
   {
       ScopedProfiler prof("Op");
       work();
   }
   Profiler::instance().print_summary();  // Shows metric

High Overhead
~~~~~~~~~~~~~

**Problem:** Profiling slows down application significantly

**Cause:** Too many fine-grained profilers

**Solution:** Profile larger operations

.. code-block:: cpp

   // AVOID - too fine-grained
   for (int i = 0; i < 100000; ++i) {
       ScopedProfiler prof("Single insert");  // Created 100k times!
       students.insert(student);
   }

   // BETTER - profile the loop
   {
       ScopedProfiler prof("Batch insert");
       for (int i = 0; i < 100000; ++i) {
           students.insert(student);
       }
       prof.set_rows_processed(100000);
   }

See Also
--------

- :doc:`query` - Query module
- :doc:`catalog` - System catalog
- :doc:`../tutorials/performance` - Performance tuning tutorial
- :doc:`../guides/optimization` - Optimization guide

**Related Headers:**

- ``learnql/debug/Profiler.hpp``
- ``learnql/debug/Statistics.hpp``
- ``learnql/debug/ExecutionPlan.hpp``
- ``learnql/debug/DebugUtils.hpp``
- ``learnql/utils/DbInspector.hpp``
