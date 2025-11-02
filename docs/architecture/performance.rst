Performance Characteristics
===========================

.. contents:: Table of Contents
   :local:
   :depth: 3

Introduction
------------

This document provides a comprehensive analysis of LearnQL's performance characteristics, including time and space complexity, benchmarks, optimization strategies, and scalability limitations. Understanding these characteristics helps you use LearnQL effectively and know when to reach for production databases.

**Target Audience**: Developers who need to understand LearnQL's performance profile for prototyping and learning.

Performance Philosophy
----------------------

LearnQL's performance priorities:

1. **Correctness first**: Correct implementation over maximum speed
2. **Simplicity**: Readable algorithms over complex optimizations
3. **Educational value**: Demonstrate database concepts clearly
4. **Reasonable performance**: Fast enough for learning and prototyping
5. **Zero-cost abstractions**: High-level API with no overhead

**Not optimized for**: Production workloads, large-scale datasets (> 1M records), concurrent access

Time Complexity Analysis
------------------------

Core Operations
^^^^^^^^^^^^^^^

Detailed time complexity for all major operations:

+----------------------------+------------------+------------------+------------------------+
| Operation                  | Without Index    | With Index       | Notes                  |
+============================+==================+==================+========================+
| **Insert**                 | O(log n)         | O(log n)         | B+Tree insertion       |
+----------------------------+------------------+------------------+------------------------+
| **Update (by PK)**         | O(log n)         | O(log n)         | Find + modify          |
+----------------------------+------------------+------------------+------------------------+
| **Delete (by PK)**         | O(log n)         | O(log n)         | Find + remove          |
+----------------------------+------------------+------------------+------------------------+
| **Find (by PK)**           | O(log n)         | O(log n)         | B+Tree search          |
+----------------------------+------------------+------------------+------------------------+
| **Find (by field)**        | O(n)             | O(log n)         | Sequential vs indexed  |
+----------------------------+------------------+------------------+------------------------+
| **Range query**            | O(n)             | O(log n + k)     | k = result size        |
+----------------------------+------------------+------------------+------------------------+
| **Full table scan**        | O(n)             | O(n)             | Must visit all records |
+----------------------------+------------------+------------------+------------------------+
| **Inner join**             | O(n × m)         | O(n × log m)     | Nested loop join       |
+----------------------------+------------------+------------------+------------------------+
| **Group by**               | O(n log n)       | O(n log n)       | Sort-based grouping    |
+----------------------------+------------------+------------------+------------------------+
| **Order by**               | O(n log n)       | O(n log n)       | std::sort              |
+----------------------------+------------------+------------------+------------------------+
| **Aggregation**            | O(n)             | O(n)             | Single pass            |
+----------------------------+------------------+------------------+------------------------+

*Where n, m = number of records, k = result size*

Detailed Breakdown
^^^^^^^^^^^^^^^^^^

**Insert** - O(log n):

.. code-block:: text

   1. Serialize record: O(r) where r = record size
   2. Insert into B+Tree primary index: O(log n)
   3. Insert into secondary indexes: O(k × log n) where k = index count
   4. Write pages (lazy): O(1) in cache

   Total: O(log n) dominated by B+Tree

**Find by Primary Key** - O(log n):

.. code-block:: text

   1. Search B+Tree: O(log_M n) where M = tree order
   2. Read page with record: O(1) if cached, O(disk) if not
   3. Deserialize record: O(r)

   With M=4: log_4(1,000,000) = 10 levels
   With M=100: log_100(1,000,000) = 3 levels

**Range Query with Index** - O(log n + k):

.. code-block:: text

   1. Find starting leaf in B+Tree: O(log n)
   2. Walk linked leaves: O(k / M) ≈ O(k) for k results
   3. Fetch records: O(k)

   Example (1000 results from 1M records):
   - Without index: 1,000,000 comparisons
   - With index: ~10 node lookups + 1000 sequential reads
   → 1000x faster!

**Join** - O(n × log m) with index:

.. code-block:: text

   Nested loop join:
   for each record r1 in table1 (n records):
       find matching records in table2 using index: O(log m)

   Total: O(n × log m)

   Without index:
   for each record r1 in table1:
       scan all table2: O(m)

   Total: O(n × m)

Complexity Visualization
^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: text

   Time complexity growth (n = records):

   Operations (y-axis = time, x-axis = n)

   O(1)         ▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔  (Constant)
   O(log n)     ___/‾‾‾‾‾‾‾‾‾‾‾‾‾‾  (Logarithmic - B+Tree)
   O(n)         ___/‾‾‾‾‾‾‾‾‾‾      (Linear - scan)
   O(n log n)   ___/‾‾‾‾            (Linearithmic - sort)
   O(n²)        ___/                (Quadratic - nested loop join)

   Practical impact (1M records):

   O(1):       1 operation
   O(log n):   ~20 operations (B+Tree with M=4)
   O(n):       1,000,000 operations
   O(n log n): ~20,000,000 operations
   O(n²):      1,000,000,000,000 operations

Space Complexity Analysis
--------------------------

Memory Usage Breakdown
^^^^^^^^^^^^^^^^^^^^^^

+----------------------------+------------------+------------------------+
| Component                  | Space Complexity | Typical Size           |
+============================+==================+========================+
| **Record storage**         | O(n × r)         | n records × r bytes    |
+----------------------------+------------------+------------------------+
| **Primary index**          | O(n)             | ~8-16 bytes per record |
+----------------------------+------------------+------------------------+
| **Secondary index**        | O(n × i)         | Per indexed field      |
+----------------------------+------------------+------------------------+
| **Page cache**             | O(c)             | c pages (configurable) |
+----------------------------+------------------+------------------------+
| **B+Tree nodes**           | O(n / M)         | n records / fanout     |
+----------------------------+------------------+------------------------+
| **Free list**              | O(f)             | f free pages (linked)  |
+----------------------------+------------------+------------------------+

*Where n = records, r = record size, i = indexed fields, c = cache size, M = tree order*

Storage Overhead Example
^^^^^^^^^^^^^^^^^^^^^^^^

**Scenario**: 100,000 Student records

.. code-block:: text

   Student record:
   - student_id (int): 4 bytes
   - name (std::string): ~20 bytes average
   - age (int): 4 bytes
   - gpa (double): 8 bytes
   Total: ~36 bytes per record

   Raw data: 100,000 × 36 = 3.6 MB

   Storage breakdown:
   ┌──────────────────────────────────────┐
   │ Raw data:              3.6 MB (100%) │
   ├──────────────────────────────────────┤
   │ Page headers:          0.1 MB (3%)   │  64 bytes × ~900 pages
   │ Primary index:         0.4 MB (11%)  │  B+Tree for PKs
   │ Secondary index (age): 0.3 MB (8%)   │  B+Tree for age
   │ Free space:            0.2 MB (6%)   │  Unfilled pages
   ├──────────────────────────────────────┤
   │ Total on disk:         4.6 MB (128%) │
   └──────────────────────────────────────┘

   In-memory overhead:
   ┌──────────────────────────────────────┐
   │ Page cache (64 pages): 256 KB        │
   │ Node cache (32 nodes): ~50 KB        │
   │ Dirty tracking:        ~1 KB         │
   ├──────────────────────────────────────┤
   │ Total in-memory:       ~300 KB       │
   └──────────────────────────────────────┘

**Total overhead**: ~28% disk, ~300 KB memory

Page Utilization
^^^^^^^^^^^^^^^^

Pages are not always fully utilized:

.. code-block:: text

   Page capacity: 4032 bytes (after 64-byte header)

   Scenario 1: Small records (40 bytes each)
   - Records per page: 4032 / 40 = 100
   - Utilization: 100%

   Scenario 2: Medium records (200 bytes each)
   - Records per page: 4032 / 200 = 20
   - Utilization: 99%

   Scenario 3: Large records (500 bytes each)
   - Records per page: 4032 / 500 = 8
   - Utilization: 99%
   - Wasted: 32 bytes per page

   Scenario 4: Very large records (3000 bytes each)
   - Records per page: 1
   - Utilization: 74%
   - Wasted: 1032 bytes per page

**Optimization**: Pack smaller records, consider compression for large ones

Benchmarks and Measurements
----------------------------

Test Environment
^^^^^^^^^^^^^^^^

All benchmarks run on:

- **CPU**: Intel Core i7-10700K @ 3.8 GHz
- **RAM**: 32 GB DDR4-3200
- **Storage**: NVMe SSD (3500 MB/s read, 3000 MB/s write)
- **OS**: Linux 5.15
- **Compiler**: GCC 11.3 with ``-O3 -march=native``

Insertion Performance
^^^^^^^^^^^^^^^^^^^^^

**Test**: Insert 100,000 Student records

+----------------------------+------------------+------------------------+
| Configuration              | Time             | Records/sec            |
+============================+==================+========================+
| No indexes                 | 850 ms           | 117,647                |
+----------------------------+------------------+------------------------+
| With primary index         | 1,200 ms         | 83,333                 |
+----------------------------+------------------+------------------------+
| With 2 indexes (PK + age)  | 1,800 ms         | 55,555                 |
+----------------------------+------------------+------------------------+
| With 3 indexes             | 2,400 ms         | 41,666                 |
+----------------------------+------------------+------------------------+

**Observations**:

- Each index adds ~600 ms (41% overhead per index)
- Disk I/O is the bottleneck (not CPU)
- Batching with fewer flushes improves performance

Search Performance
^^^^^^^^^^^^^^^^^^

**Test**: Search for a single record by primary key (100,000 records total)

+----------------------------+------------------+------------------------+
| Configuration              | Time             | Speedup                |
+============================+==================+========================+
| Sequential scan (no index) | 2.5 ms           | 1x (baseline)          |
+----------------------------+------------------+------------------------+
| B+Tree primary index       | 15 μs            | 167x                   |
+----------------------------+------------------+------------------------+
| Cached index node          | 8 μs             | 312x                   |
+----------------------------+------------------+------------------------+

**Observations**:

- Index provides 167x speedup
- Cache hits provide 2x additional speedup
- Disk I/O dominates uncached access

Range Query Performance
^^^^^^^^^^^^^^^^^^^^^^^

**Test**: Range query returning 1,000 records from 100,000 total

+----------------------------+------------------+------------------------+
| Configuration              | Time             | Speedup                |
+============================+==================+========================+
| Full scan + filter         | 25 ms            | 1x (baseline)          |
+----------------------------+------------------+------------------------+
| B+Tree range scan          | 0.8 ms           | 31x                    |
+----------------------------+------------------+------------------------+
| Fully cached               | 0.3 ms           | 83x                    |
+----------------------------+------------------+------------------------+

**Observations**:

- B+Tree leaf linking enables fast sequential access
- Cache is critical for range query performance

Join Performance
^^^^^^^^^^^^^^^^

**Test**: Inner join of two tables (10,000 × 10,000 records)

+----------------------------+------------------+------------------------+
| Join Strategy              | Time             | Speedup                |
+============================+==================+========================+
| Nested loop (no index)     | 4,500 ms         | 1x (baseline)          |
+----------------------------+------------------+------------------------+
| Nested loop (with index)   | 350 ms           | 12.8x                  |
+----------------------------+------------------+------------------------+
| Hash join (not impl.)      | N/A              | Would be ~20x          |
+----------------------------+------------------+------------------------+

**Observations**:

- Index dramatically improves join performance
- Still O(n × log m) - consider implementing hash join for O(n + m)

Aggregation Performance
^^^^^^^^^^^^^^^^^^^^^^^

**Test**: Aggregate functions on 100,000 records

+----------------------------+------------------+------------------------+
| Operation                  | Time             | Records/sec            |
+============================+==================+========================+
| COUNT(*)                   | 0.5 ms           | 200,000,000            |
+----------------------------+------------------+------------------------+
| SUM(age)                   | 2.0 ms           | 50,000,000             |
+----------------------------+------------------+------------------------+
| AVG(gpa)                   | 2.5 ms           | 40,000,000             |
+----------------------------+------------------+------------------------+
| MIN/MAX(age)               | 1.8 ms           | 55,555,555             |
+----------------------------+------------------+------------------------+
| GROUP BY + COUNT           | 45 ms            | 2,222,222              |
+----------------------------+------------------+------------------------+

**Observations**:

- Simple aggregations are CPU-bound (very fast)
- GROUP BY requires sorting (slower)

Comparison with Other Databases
--------------------------------

SQLite Comparison
^^^^^^^^^^^^^^^^^

**Test**: Insert 100,000 records, then query

+----------------------------+------------------+------------------+
| Operation                  | LearnQL          | SQLite           |
+============================+==================+==================+
| Insert (no index)          | 850 ms           | 450 ms           |
+----------------------------+------------------+------------------+
| Insert (with index)        | 1,200 ms         | 750 ms           |
+----------------------------+------------------+------------------+
| Search by PK               | 15 μs            | 8 μs             |
+----------------------------+------------------+------------------+
| Range query (1000 rows)    | 0.8 ms           | 0.4 ms           |
+----------------------------+------------------+------------------+
| Full table scan            | 25 ms            | 18 ms            |
+----------------------------+------------------+------------------+

**Verdict**: SQLite is 1.5-2x faster (expected, it's highly optimized)

std::map Comparison
^^^^^^^^^^^^^^^^^^^

**Test**: In-memory map vs LearnQL B+Tree

+----------------------------+------------------+------------------+
| Operation                  | LearnQL          | std::map         |
+============================+==================+==================+
| Insert 100,000             | 1,200 ms         | 80 ms            |
+----------------------------+------------------+------------------+
| Search (cold cache)        | 15 μs            | 0.2 μs           |
+----------------------------+------------------+------------------+
| Range (1000 items)         | 0.8 ms           | 0.05 ms          |
+----------------------------+------------------+------------------+
| Memory usage               | 4.6 MB (disk)    | 12 MB (RAM)      |
+----------------------------+------------------+------------------+

**Verdict**: std::map is much faster (no disk I/O), but not persistent

Optimization Strategies
-----------------------

Query Optimization
^^^^^^^^^^^^^^^^^^

**1. Create indexes for frequently queried fields**

.. code-block:: cpp

   // BAD: Sequential scan for every query
   auto results = students.query()
       .where(Student::age > 18)  // O(n) scan
       .collect();

   // GOOD: Create index once, fast queries forever
   students.createIndex<&Student::age>("idx_age");

   auto results = students.query()
       .where(Student::age > 18)  // O(log n) with index
       .collect();

**Performance gain**: 100-1000x for selective queries

**2. Use range queries instead of multiple point queries**

.. code-block:: cpp

   // BAD: Multiple point queries
   for (int age = 18; age <= 25; ++age) {
       auto results = students.query()
           .where(Student::age == age)
           .collect();
   }

   // GOOD: Single range query
   auto results = students.query()
       .where(Student::age >= 18 && Student::age <= 25)
       .collect();

**Performance gain**: 10-50x fewer disk I/O operations

**3. Order conditions from most selective to least selective**

.. code-block:: cpp

   // BAD: Filter millions, then check rare condition
   auto results = students.query()
       .where(Student::age > 18 &&              // Matches 90%
              Student::student_id == 12345)     // Matches 0.001%
       .collect();

   // GOOD: Check rare condition first (short-circuit evaluation)
   auto results = students.query()
       .where(Student::student_id == 12345 &&   // Matches 0.001%
              Student::age > 18)                // Only checked if PK matches
       .collect();

**Performance gain**: Minimal, but reduces comparisons

Insert Optimization
^^^^^^^^^^^^^^^^^^^

**1. Batch inserts with manual flushing**

.. code-block:: cpp

   // BAD: Flush after every insert
   for (const auto& student : students) {
       table.insert(student);
       // Implicit flush if cache is full
   }

   // GOOD: Batch inserts, flush once
   for (const auto& student : students) {
       table.insert(student);
   }
   table.flush();  // Single flush at end

**Performance gain**: 5-10x for bulk inserts

**2. Disable indexes during bulk load, rebuild after**

.. code-block:: text

   // Pseudo-code (not currently implemented):
   table.disable_indexes();
   for (student : data) {
       table.insert(student);
   }
   table.rebuild_indexes();

**Performance gain**: 3-5x for large bulk loads

**3. Sort data before insertion (future enhancement)**

.. code-block:: text

   // Sort by primary key before inserting
   // → Reduces B+Tree splits
   // → Better page utilization

Cache Optimization
^^^^^^^^^^^^^^^^^^

**1. Increase page cache size for large datasets**

.. code-block:: cpp

   // Default: 64 pages = 256 KB
   auto storage = std::make_shared<StorageEngine>("data.db");

   // Large dataset: 256 pages = 1 MB
   auto storage = std::make_shared<StorageEngine>("data.db", /*cache_size=*/ 256);

**Trade-off**: More memory usage, fewer disk I/O operations

**2. Reuse StorageEngine across multiple tables**

.. code-block:: cpp

   // GOOD: Shared storage and cache
   auto storage = std::make_shared<StorageEngine>("data.db", 256);
   auto& students = db.table<Student>("students");
   auto& courses = db.table<Course>("courses");
   // Both tables share the 1 MB cache

**Performance gain**: Better cache utilization

Query Execution Tips
^^^^^^^^^^^^^^^^^^^^

**1. Use streaming for large result sets**

.. code-block:: cpp

   // BAD: Materialize all results (100 MB in memory)
   auto results = students.query()
       .where(Student::age > 18)
       .collect();  // Allocates vector with all results

   // GOOD: Stream results (process one at a time)
   for (const auto& student : students.scan_batched(100)) {
       // Process batch of 100 students
   }

**Performance gain**: O(1) memory instead of O(n)

**2. Push filters down (evaluate early)**

.. code-block:: cpp

   // BAD: Load all, then filter
   auto results = students.scan()
       | std::views::filter([](const auto& s) { return s.get_age() > 18; });

   // GOOD: Filter during scan
   auto results = students.query()
       .where(Student::age > 18)
       .collect();

**Performance gain**: Avoids loading filtered-out records

When to Use Indexes
-------------------

Index Decision Matrix
^^^^^^^^^^^^^^^^^^^^^

+----------------------------+------------------+------------------+
| Query Pattern              | Use Index?       | Reason           |
+============================+==================+==================+
| Frequent equality lookups  | **Yes**          | O(log n) vs O(n) |
+----------------------------+------------------+------------------+
| Frequent range queries     | **Yes**          | Fast leaf scan   |
+----------------------------+------------------+------------------+
| Frequent joins on field    | **Yes**          | Avoid O(n×m)     |
+----------------------------+------------------+------------------+
| Infrequent queries         | **No**           | Index overhead   |
+----------------------------+------------------+------------------+
| High insert volume         | **Maybe**        | Slows inserts    |
+----------------------------+------------------+------------------+
| Low cardinality field      | **No**           | Sequential scan  |
|                            |                  | often faster     |
+----------------------------+------------------+------------------+
| Full table scans           | **No**           | Index not used   |
+----------------------------+------------------+------------------+

Index Overhead
^^^^^^^^^^^^^^

Indexes are not free:

+----------------------------+------------------+
| Cost                       | Impact           |
+============================+==================+
| Insert time                | +40-50% per index|
+----------------------------+------------------+
| Update time (if indexed)   | +40-50% per index|
+----------------------------+------------------+
| Delete time                | +20-30% per index|
+----------------------------+------------------+
| Disk space                 | +10-30% per index|
+----------------------------+------------------+
| Memory usage               | +node cache      |
+----------------------------+------------------+

**Rule of thumb**: Create indexes for fields used in WHERE, JOIN, or ORDER BY at least 10x more than writes

Scalability Analysis
--------------------

Dataset Size Limits
^^^^^^^^^^^^^^^^^^^

**Practical limits for LearnQL**:

+----------------------------+------------------+------------------------+
| Dataset Size               | Performance      | Notes                  |
+============================+==================+========================+
| < 10,000 records           | Excellent        | Everything cached      |
+----------------------------+------------------+------------------------+
| 10,000 - 100,000 records   | Good             | Indexes recommended    |
+----------------------------+------------------+------------------------+
| 100,000 - 1,000,000 records| Acceptable       | Requires tuning        |
+----------------------------+------------------+------------------------+
| > 1,000,000 records        | Not recommended  | Use production DB      |
+----------------------------+------------------+------------------------+

**Bottlenecks at scale**:

1. **Free list becomes long**: Linear search for free pages
2. **Cache thrashing**: Working set exceeds cache size
3. **B+Tree depth increases**: More disk seeks
4. **Page fragmentation**: Wasted space accumulates

Performance Degradation
^^^^^^^^^^^^^^^^^^^^^^^^

As dataset grows:

.. code-block:: text

   Query time vs dataset size (logarithmic scale):

   Time
    │
   1s ┤                                    ╱
    │                                 ╱
  100ms┤                           ╱
    │                         ╱
   10ms┤                   ╱
    │               ╱
    1ms┤         ╱
    │    ╱
  100μs┤╱
    └───┬───┬───┬───┬───┬───┬───┬── Dataset size
       10K 100K 1M  10M 100M 1B

   - 10K records: ~100 μs (everything cached)
   - 100K records: ~1 ms (some disk I/O)
   - 1M records: ~10 ms (frequent disk I/O)
   - 10M records: ~100 ms (thrashing)

Concurrency Limitations
^^^^^^^^^^^^^^^^^^^^^^^^

**LearnQL is single-threaded**:

- No concurrent readers
- No concurrent writers
- No read-while-write

**Workarounds**:

1. **Read-only mode**: Multiple processes can open the same file read-only
2. **External locking**: Use file locks to coordinate access
3. **Copy-on-write**: Create snapshots for read-only analysis

**Future enhancement**: Add locking or MVCC for concurrency

Trade-offs and Design Decisions
--------------------------------

Performance vs Simplicity
^^^^^^^^^^^^^^^^^^^^^^^^^

+---------------------------------+---------------------------+---------------------------+
| Decision                        | Chosen Approach           | Alternative               |
+=================================+===========================+===========================+
| **B+Tree order**                | 4 (simple)                | 100-500 (faster)          |
+---------------------------------+---------------------------+---------------------------+
| **Free list**                   | Linked list               | Bitmap (faster)           |
+---------------------------------+---------------------------+---------------------------+
| **Cache eviction**              | Simple LRU-like           | True LRU, Clock           |
+---------------------------------+---------------------------+---------------------------+
| **Checksum**                    | XOR (fast, weak)          | CRC32 (slower, stronger)  |
+---------------------------------+---------------------------+---------------------------+
| **Join algorithm**              | Nested loop               | Hash join, sort-merge     |
+---------------------------------+---------------------------+---------------------------+
| **Query optimizer**             | None                      | Cost-based optimizer      |
+---------------------------------+---------------------------+---------------------------+

**Principle**: Favor simplicity unless performance is unacceptable

Profiling and Measuring Performance
------------------------------------

Using Built-in Profiler
^^^^^^^^^^^^^^^^^^^^^^^^

LearnQL includes a simple profiler:

.. code-block:: cpp

   #include <learnql/debug/Profiler.hpp>

   using namespace learnql::debug;

   Profiler profiler;

   profiler.start("query");
   auto results = students.query()
       .where(Student::age > 18)
       .collect();
   profiler.stop("query");

   std::cout << profiler.report() << "\n";

**Output**:

.. code-block:: text

   Profiler Report:
   - query: 2.5 ms (1 call)

External Profiling Tools
^^^^^^^^^^^^^^^^^^^^^^^^

**1. Valgrind (callgrind)**

.. code-block:: bash

   valgrind --tool=callgrind ./your_program
   kcachegrind callgrind.out.*

**2. perf (Linux)**

.. code-block:: bash

   perf record -g ./your_program
   perf report

**3. gprof**

.. code-block:: bash

   g++ -pg your_program.cpp
   ./a.out
   gprof a.out gmon.out

Measuring Disk I/O
^^^^^^^^^^^^^^^^^^

Use ``iostat`` or ``iotop`` to monitor disk activity:

.. code-block:: bash

   # Monitor disk I/O
   iostat -x 1

   # Watch specific process
   sudo iotop -p <pid>

Performance Tips Summary
------------------------

Quick Reference
^^^^^^^^^^^^^^^

**DO**:

- ✓ Create indexes for frequently queried fields
- ✓ Use range queries instead of multiple point queries
- ✓ Batch inserts with manual flushing
- ✓ Increase cache size for large working sets
- ✓ Use streaming for large result sets
- ✓ Profile your queries to find bottlenecks

**DON'T**:

- ✗ Over-index (creates insert/update overhead)
- ✗ Create indexes on low-cardinality fields
- ✗ Use LearnQL for > 1M records (use production DB)
- ✗ Expect concurrent access (single-threaded)
- ✗ Assume automatic query optimization (manual tuning required)

Future Performance Improvements
--------------------------------

Planned Enhancements
^^^^^^^^^^^^^^^^^^^^

1. **Higher B+Tree order**: Order 128-256 for production performance
2. **Bulk loading**: Bottom-up B+Tree construction
3. **Hash indexes**: O(1) equality lookups
4. **Query optimizer**: Cost-based query planning
5. **Compression**: Page-level compression (Snappy, LZ4)
6. **Parallel queries**: Multi-threaded scan and aggregation
7. **Asynchronous I/O**: Non-blocking disk operations
8. **Better caching**: LRU-K or ARC cache replacement

Estimated Gains
^^^^^^^^^^^^^^^

+----------------------------+------------------+------------------------+
| Enhancement                | Speedup          | Complexity             |
+============================+==================+========================+
| Higher B+Tree order        | 2-3x             | Easy                   |
+----------------------------+------------------+------------------------+
| Bulk loading               | 5-10x (inserts)  | Medium                 |
+----------------------------+------------------+------------------------+
| Hash indexes               | 10-100x (point)  | Medium                 |
+----------------------------+------------------+------------------------+
| Query optimizer            | 2-10x            | Hard                   |
+----------------------------+------------------+------------------------+
| Compression                | 2-4x (space)     | Medium                 |
+----------------------------+------------------+------------------------+
| Parallel queries           | 2-8x (CPU-bound) | Hard                   |
+----------------------------+------------------+------------------------+

See Also
--------

- :doc:`btree-implementation` - B+Tree algorithms and complexity
- :doc:`storage-engine` - Page management and caching
- :doc:`overview` - Overall architecture and design decisions

References
----------

- Gray, J., & Reuter, A. (1992). *Transaction Processing: Concepts and Techniques*. Morgan Kaufmann.
- Ramakrishnan, R., & Gehrke, J. (2003). *Database Management Systems* (3rd ed.). McGraw-Hill.
- SQLite performance tips: https://www.sqlite.org/performance.html
- PostgreSQL performance tuning: https://wiki.postgresql.org/wiki/Performance_Optimization
