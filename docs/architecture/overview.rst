Architecture Overview
=====================

.. contents:: Table of Contents
   :local:
   :depth: 3

Introduction
------------

LearnQL is a modern C++20 database implementation designed for educational purposes. This document provides a comprehensive overview of the system architecture, design decisions, and component interactions. The architecture follows a **layered design** with clear separation of concerns, enabling learners to understand database internals incrementally.

**Target Audience**: C++ developers who want to understand how databases work internally, from storage engines to query processing.

System Architecture
-------------------

High-Level Architecture Diagram
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: text

   ┌─────────────────────────────────────────────────────────────┐
   │                     APPLICATION LAYER                        │
   │          (User Code using LearnQL API)                       │
   └───────────────────────────┬─────────────────────────────────┘
                               │
   ┌───────────────────────────▼─────────────────────────────────┐
   │                      API LAYER                               │
   │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │
   │  │   Database   │  │    Table     │  │    Query     │       │
   │  │   (facade)   │  │ (operations) │  │ (DSL entry)  │       │
   │  └──────────────┘  └──────────────┘  └──────────────┘       │
   └───────────────────────────┬─────────────────────────────────┘
                               │
   ┌───────────────────────────▼─────────────────────────────────┐
   │                    QUERY LAYER                               │
   │  ┌────────────────────────────────────────────────┐          │
   │  │   Expression Templates (Compile-Time AST)      │          │
   │  │   FieldExpr, ConstExpr, BinaryExpr, LogicalExpr│          │
   │  └────────────────────────────────────────────────┘          │
   │  ┌────────────────────────────────────────────────┐          │
   │  │   Query Execution (Runtime Evaluation)         │          │
   │  │   Filter, Join, GroupBy, OrderBy               │          │
   │  └────────────────────────────────────────────────┘          │
   └───────────────────────────┬─────────────────────────────────┘
                               │
   ┌───────────────────────────▼─────────────────────────────────┐
   │                    INDEX LAYER                               │
   │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │
   │  │  Primary Key │  │   Secondary  │  │   B+Tree     │       │
   │  │    Index     │  │    Index     │  │ Implementation│       │
   │  └──────────────┘  └──────────────┘  └──────────────┘       │
   └───────────────────────────┬─────────────────────────────────┘
                               │
   ┌───────────────────────────▼─────────────────────────────────┐
   │                   STORAGE LAYER                              │
   │  ┌────────────────────────────────────────────────┐          │
   │  │   Page Management (4KB pages)                  │          │
   │  │   Free List, Page Cache, Dirty Tracking        │          │
   │  └────────────────────────────────────────────────┘          │
   │  ┌────────────────────────────────────────────────┐          │
   │  │   Record Storage (Serialization)               │          │
   │  │   BinaryWriter/Reader                          │          │
   │  └────────────────────────────────────────────────┘          │
   └───────────────────────────┬─────────────────────────────────┘
                               │
   ┌───────────────────────────▼─────────────────────────────────┐
   │                  FILE SYSTEM LAYER                           │
   │              (Database File: *.db)                           │
   └─────────────────────────────────────────────────────────────┘


Layered Architecture
^^^^^^^^^^^^^^^^^^^^

LearnQL follows a **strict layered architecture** where each layer only depends on layers below it:

1. **Application Layer**: User code that uses the LearnQL API
2. **API Layer**: High-level database and table abstractions
3. **Query Layer**: Expression templates and query execution
4. **Index Layer**: B+Tree indexes for fast lookups
5. **Storage Layer**: Page-based storage engine with caching
6. **File System Layer**: Physical disk storage

Design Principles
-----------------

1. Zero-Cost Abstractions
^^^^^^^^^^^^^^^^^^^^^^^^^

LearnQL heavily uses C++20 features to achieve **zero-cost abstractions**:

- **Expression Templates**: Query conditions are built at compile-time without runtime overhead
- **Concepts**: Type constraints are checked at compile-time
- **Compile-Time Reflection**: Metadata generation happens during compilation
- **Template Metaprogramming**: Property macros generate code at compile-time

**Example**: A query like ``Student::age > 18 && Student::gpa > 3.5`` creates a compile-time expression tree. No virtual functions, no dynamic dispatch, no runtime type checking.

2. Type Safety
^^^^^^^^^^^^^^

Everything is type-checked at compile-time:

- **Field access**: Can only query fields that exist on the type
- **Type matching**: Join conditions must compare compatible types
- **Concept constraints**: Template parameters are validated using C++20 concepts

**Example**: This won't compile (caught at compile-time):

.. code-block:: cpp

   // Error: std::string and int are not comparable
   auto results = students.where(Student::name > 42);  // Compile error!

3. Educational Focus
^^^^^^^^^^^^^^^^^^^^

The architecture is designed to be **readable and understandable**:

- **Clear separation of concerns**: Each component has a single responsibility
- **Extensive documentation**: Every class and method is documented
- **Simple algorithms**: Uses textbook implementations rather than heavily optimized code
- **Progressive complexity**: Basic features are simple, advanced features are optional

4. RAII and Modern C++
^^^^^^^^^^^^^^^^^^^^^^

Follows modern C++ best practices:

- **RAII**: All resources (files, memory) are managed automatically
- **Move semantics**: Efficient transfer of ownership
- **Smart pointers**: No manual memory management
- **``std::span``**: Safe views into memory
- **``constexpr``**: Compile-time computation where possible

Component Interactions
----------------------

Data Flow Example: Query Execution
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Let's trace how a query flows through the system:

.. code-block:: cpp

   auto results = students.where(Student::age > 18).collect();

**Step-by-step execution**:

1. **Expression Template Construction** (Compile-Time)

   - ``Student::age > 18`` creates ``BinaryExpr<Greater, FieldExpr<int>, ConstExpr<int>>``
   - Type is determined at compile-time
   - No evaluation happens yet

2. **Query Builder** (Runtime)

   - ``where()`` stores the expression
   - Returns the table view for iteration

3. **Filter Phase** (Runtime)

   - Scans all records in the table
   - For each record, calls ``expression.evaluate(record)``
   - Only matching records are kept

4. **Collection** (Runtime)

   - ``collect()`` materializes results into a ``std::vector``
   - Returns owned data to user

Module Dependencies
-------------------

Dependency Graph
^^^^^^^^^^^^^^^^

.. code-block:: text

   core::Database
     ├── storage::StorageEngine (owns)
     ├── catalog::SystemCatalog (owns)
     └── core::Table<T> (creates)
           ├── storage::StorageEngine (shared_ptr)
           ├── index::PersistentBTreeIndex<PK, RecordId> (owns)
           └── index::PersistentSecondaryIndex<Key, PK> (owns, optional)

   query::Query<T>
     ├── core::Table<T> (reference)
     └── expressions::Expr<Derived> (value)
           ├── FieldExpr<T, FieldType>
           ├── ConstExpr<Value>
           ├── BinaryExpr<Op, Left, Right>
           └── LogicalExpr<Op, Left, Right>

   storage::StorageEngine
     ├── storage::Page (manages)
     └── std::fstream (owns)

   index::PersistentBTreeIndex<K, V>
     ├── storage::StorageEngine (shared_ptr)
     └── storage::Page (reads/writes)

**Dependency Rules**:

- **No circular dependencies**: Clean acyclic dependency graph
- **Interface segregation**: Components depend on abstractions, not implementations
- **Shared ownership**: ``StorageEngine`` is shared via ``shared_ptr``
- **Value semantics**: Expression templates are passed by value (cheap to copy)

Why C++20?
----------

LearnQL leverages specific C++20 features for educational and practical reasons:

Concepts
^^^^^^^^

**Why**: Type constraints with clear error messages

**Usage in LearnQL**:

.. code-block:: cpp

   template<typename T>
   concept Serializable = requires(T obj, BinaryWriter& writer, BinaryReader& reader) {
       { obj.serialize(writer) } -> std::same_as<void>;
       { obj.deserialize(reader) } -> std::same_as<void>;
   };

**Benefit**: Clear compile-time errors instead of cryptic template instantiation failures.

Ranges
^^^^^^

**Why**: Composable, lazy operations on sequences

**Usage in LearnQL**:

.. code-block:: cpp

   auto top_students = students.where(Student::gpa > 3.5)
       | std::views::take(10)
       | std::views::transform([](const auto& s) { return s.get_name(); });

**Benefit**: Can compose database queries with standard library algorithms.

Coroutines
^^^^^^^^^^

**Why**: Generators for memory-efficient streaming

**Usage in LearnQL**:

.. code-block:: cpp

   coroutines::Generator<Student> streamStudents(Table<Student>& table) {
       for (const auto& student : table) {
           co_yield student;
       }
   }

**Benefit**: Process large result sets without loading everything into memory.

Three-Way Comparison (``operator<=>``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Why**: Generates all comparison operators from one definition

**Usage in LearnQL**:

.. code-block:: cpp

   struct PageHeader {
       // ... members ...
       auto operator<=>(const PageHeader&) const noexcept = default;
   };

**Benefit**: Less boilerplate, consistent comparison semantics.

``std::span``
^^^^^^^^^^^^^

**Why**: Safe, non-owning view into contiguous memory

**Usage in LearnQL**:

.. code-block:: cpp

   std::span<uint8_t> data() noexcept {
       return std::span<uint8_t>(data_.data(), data_.size());
   }

**Benefit**: Avoid buffer overruns, clear ownership semantics.

Comparison with Traditional Databases
--------------------------------------

LearnQL vs SQLite
^^^^^^^^^^^^^^^^^

+------------------------+-------------------------+-------------------------+
| Feature                | LearnQL                 | SQLite                  |
+========================+=========================+=========================+
| **Language**           | C++20 header-only       | C library               |
+------------------------+-------------------------+-------------------------+
| **Query Interface**    | Type-safe expression    | String-based SQL        |
|                        | templates               |                         |
+------------------------+-------------------------+-------------------------+
| **Schema Definition**  | C++ structs with macros | CREATE TABLE statements |
+------------------------+-------------------------+-------------------------+
| **Type Safety**        | Compile-time            | Runtime                 |
+------------------------+-------------------------+-------------------------+
| **Transactions**       | None                    | ACID with WAL           |
+------------------------+-------------------------+-------------------------+
| **Concurrency**        | Single-threaded         | Multi-reader,           |
|                        |                         | single-writer           |
+------------------------+-------------------------+-------------------------+
| **Storage Format**     | Simple page-based       | Sophisticated B-Tree    |
|                        |                         | with overflow pages     |
+------------------------+-------------------------+-------------------------+
| **Query Optimizer**    | None (sequential scan)  | Cost-based optimizer    |
+------------------------+-------------------------+-------------------------+
| **Index Types**        | B+Tree only             | B-Tree, R-Tree, FTS     |
+------------------------+-------------------------+-------------------------+
| **Network Protocol**   | None (in-process)       | None (in-process)       |
+------------------------+-------------------------+-------------------------+
| **Use Case**           | Learning & prototyping  | Production embedded DB  |
+------------------------+-------------------------+-------------------------+

LearnQL vs PostgreSQL
^^^^^^^^^^^^^^^^^^^^^^

+------------------------+-------------------------+-------------------------+
| Feature                | LearnQL                 | PostgreSQL              |
+========================+=========================+=========================+
| **Architecture**       | Embedded library        | Client-server           |
+------------------------+-------------------------+-------------------------+
| **Language**           | C++20 only              | C (extensible in many)  |
+------------------------+-------------------------+-------------------------+
| **Query Language**     | C++ expression          | SQL (with extensions)   |
|                        | templates               |                         |
+------------------------+-------------------------+-------------------------+
| **ACID Support**       | No transactions         | Full ACID with MVCC     |
+------------------------+-------------------------+-------------------------+
| **Indexing**           | B+Tree only             | B-Tree, GiST, GIN,      |
|                        |                         | BRIN, Hash              |
+------------------------+-------------------------+-------------------------+
| **Query Planner**      | None                    | Sophisticated optimizer |
+------------------------+-------------------------+-------------------------+
| **Scalability**        | < 1M records            | Billions of records     |
+------------------------+-------------------------+-------------------------+
| **Replication**        | None                    | Streaming, logical      |
+------------------------+-------------------------+-------------------------+
| **Use Case**           | Learning & prototyping  | Production enterprise   |
|                        |                         | database                |
+------------------------+-------------------------+-------------------------+

Full System Architecture Diagram
---------------------------------

Complete Component Interaction Map
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. mermaid::

   graph TB
       subgraph "User Application"
           APP[User Code]
       end

       subgraph "Core API"
           DB[Database]
           TBL[Table<T>]
           QUERY[Query<T>]
       end

       subgraph "Query Processing"
           EXPR[Expression Templates]
           EVAL[Expression Evaluator]
           FILTER[Filter Operator]
           JOIN[Join Operator]
           GROUPBY[GroupBy Operator]
           ORDERBY[OrderBy Operator]
       end

       subgraph "Metadata & Reflection"
           CATALOG[SystemCatalog]
           PROPERTY[Property Macros]
           REFLECTION[Field Reflection]
       end

       subgraph "Index Management"
           PKIDX[Primary Key Index]
           SECIDX[Secondary Index]
           BTREE[B+Tree Implementation]
       end

       subgraph "Storage Engine"
           STORAGE[StorageEngine]
           PAGE[Page Manager]
           CACHE[Page Cache]
           FREELIST[Free List]
       end

       subgraph "Serialization"
           BINWRITE[BinaryWriter]
           BINREAD[BinaryReader]
       end

       subgraph "File System"
           DBFILE[(database.db)]
       end

       APP --> DB
       APP --> TBL
       APP --> QUERY

       DB --> STORAGE
       DB --> CATALOG

       TBL --> STORAGE
       TBL --> PKIDX
       TBL --> SECIDX
       TBL --> QUERY

       QUERY --> EXPR
       QUERY --> EVAL

       EVAL --> FILTER
       EVAL --> JOIN
       EVAL --> GROUPBY
       EVAL --> ORDERBY

       PROPERTY --> REFLECTION
       REFLECTION --> CATALOG

       PKIDX --> BTREE
       SECIDX --> BTREE
       BTREE --> STORAGE

       STORAGE --> PAGE
       STORAGE --> CACHE
       STORAGE --> FREELIST

       PAGE --> BINWRITE
       PAGE --> BINREAD

       STORAGE --> DBFILE

       style APP fill:#e1f5e1
       style DB fill:#ffe1e1
       style TBL fill:#ffe1e1
       style STORAGE fill:#e1e5ff
       style BTREE fill:#fff5e1
       style EXPR fill:#f5e1ff

Key Design Decisions
--------------------

1. Header-Only Library
^^^^^^^^^^^^^^^^^^^^^^^

**Decision**: Implement LearnQL as a header-only library

**Rationale**:

- **Simplicity**: No build system complexity for users
- **Inline optimization**: Compiler can inline across translation units
- **Template-heavy**: Most code is template-based anyway
- **Educational**: Easy to explore source code

**Trade-off**: Longer compilation times

2. Page-Based Storage (4KB pages)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Decision**: Use fixed 4KB pages for all storage

**Rationale**:

- **OS alignment**: Matches typical file system block size
- **Simplicity**: Fixed-size allocation is easier to implement
- **Cache-friendly**: Fits in CPU cache lines
- **Standard practice**: Most databases use page-based storage

**Trade-off**: Can waste space for small records

3. B+Tree (not B-Tree)
^^^^^^^^^^^^^^^^^^^^^^

**Decision**: Use B+Tree for indexes, not classic B-Tree

**Rationale**:

- **Better range queries**: Leaf nodes are linked, enabling sequential scans
- **Higher fanout**: Internal nodes store only keys, not values
- **Consistent performance**: All queries go to leaf level
- **Modern standard**: Most database indexes are B+Trees

**Trade-off**: Slightly more complex than B-Tree

4. Compile-Time Expression Templates
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Decision**: Build query expressions at compile-time using templates

**Rationale**:

- **Zero overhead**: No virtual function calls or dynamic dispatch
- **Type safety**: Catch type errors at compile-time
- **Educational**: Demonstrates advanced C++ techniques
- **Performance**: As fast as hand-written loops

**Trade-off**: Complex template error messages (mitigated by concepts)

5. No Query Optimizer
^^^^^^^^^^^^^^^^^^^^^^

**Decision**: Execute queries in the order specified by user

**Rationale**:

- **Simplicity**: Query optimization is a complex topic
- **Predictability**: Users know exactly what will happen
- **Educational focus**: Can add optimization as an advanced topic
- **Index hints**: Users can explicitly create indexes

**Trade-off**: Users must manually optimize queries

File Format Specification
--------------------------

Database File Structure
^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: text

   database.db
   ┌──────────────────────────────────────────┐
   │ Page 0: Metadata Page (4096 bytes)       │  ← Database header
   ├──────────────────────────────────────────┤
   │ Page 1: Data or Index Page               │
   ├──────────────────────────────────────────┤
   │ Page 2: Data or Index Page               │
   ├──────────────────────────────────────────┤
   │ ...                                      │
   ├──────────────────────────────────────────┤
   │ Page N: Data or Index Page               │
   └──────────────────────────────────────────┘

**Page 0 Layout** (Metadata Page):

.. code-block:: text

   Offset   Size    Description
   ──────────────────────────────────────────────
   0-15     16 B    Magic: "LearnQL Database"
   16-23    8 B     next_page_id (uint64_t)
   24-31    8 B     free_list_head (uint64_t)
   32-39    8 B     sys_tables_root (uint64_t)
   40-47    8 B     sys_fields_root (uint64_t)
   48-51    4 B     version (uint32_t) = 3
   52-59    8 B     created_timestamp (uint64_t)
   60-67    8 B     sys_indexes_root (uint64_t)
   68-4095  ...     Reserved for future use

**Regular Page Layout** (Pages 1+):

.. code-block:: text

   ┌─────────────────────────────────┐
   │ Page Header (64 bytes)          │
   ├─────────────────────────────────┤
   │                                 │
   │ Data Section (4032 bytes)       │
   │                                 │
   └─────────────────────────────────┘

See :doc:`storage-engine` for detailed page structure.

Performance Characteristics
---------------------------

Time Complexity (Typical)
^^^^^^^^^^^^^^^^^^^^^^^^^^

+------------------------+------------------+------------------------+
| Operation              | Without Index    | With Index             |
+========================+==================+========================+
| Insert                 | O(log n)         | O(log n)               |
+------------------------+------------------+------------------------+
| Update (by PK)         | O(log n)         | O(log n)               |
+------------------------+------------------+------------------------+
| Delete (by PK)         | O(log n)         | O(log n)               |
+------------------------+------------------+------------------------+
| Find (by PK)           | O(log n)         | O(log n)               |
+------------------------+------------------+------------------------+
| Find (by field)        | O(n)             | O(log n)               |
+------------------------+------------------+------------------------+
| Range query            | O(n)             | O(log n + k)           |
+------------------------+------------------+------------------------+
| Full table scan        | O(n)             | O(n)                   |
+------------------------+------------------+------------------------+
| Join (nested loop)     | O(n × m)         | O(n × log m)           |
+------------------------+------------------+------------------------+
| GroupBy                | O(n log n)       | O(n log n)             |
+------------------------+------------------+------------------------+
| OrderBy                | O(n log n)       | O(n log n)             |
+------------------------+------------------+------------------------+

*Where n, m, k are the number of records*

Space Complexity
^^^^^^^^^^^^^^^^

- **Per record**: ~overhead of 16-32 bytes per record (record header)
- **Per page**: 64 bytes header + 4032 bytes data
- **Page cache**: Configurable (default: 64 pages = 256 KB)
- **Index**: ~50-70% of indexed column size

See :doc:`performance` for detailed analysis and benchmarks.

Limitations and Trade-offs
---------------------------

Current Limitations
^^^^^^^^^^^^^^^^^^^

1. **Single-threaded**: No concurrent access support

   - **Reason**: Concurrency control is complex (locking, MVCC)
   - **Mitigation**: Could add read-only mode with shared locks

2. **No transactions**: No ACID guarantees

   - **Reason**: Requires write-ahead logging (WAL), undo logs
   - **Mitigation**: Manual ``flush()`` for durability points

3. **No query optimizer**: Executes queries as written

   - **Reason**: Query optimization is a large topic
   - **Mitigation**: Users create indexes manually

4. **Limited scalability**: Best for < 1M records

   - **Reason**: Simple free list, no page splitting strategies
   - **Mitigation**: Use for prototyping, not production

5. **No networking**: In-process only

   - **Reason**: Network protocols are outside scope
   - **Mitigation**: Could add RPC layer separately

Design Trade-offs
^^^^^^^^^^^^^^^^^

+---------------------------------+---------------------------+---------------------------+
| Trade-off                       | Chosen Approach           | Alternative               |
+=================================+===========================+===========================+
| **Simplicity vs Performance**   | Favor simplicity          | Highly optimized code     |
+---------------------------------+---------------------------+---------------------------+
| **Compile-time vs Runtime**     | Compile-time checks       | Runtime flexibility       |
+---------------------------------+---------------------------+---------------------------+
| **Type safety vs Convenience**  | Strong type safety        | Loosely-typed API         |
+---------------------------------+---------------------------+---------------------------+
| **Header-only vs Library**      | Header-only               | Compiled library          |
+---------------------------------+---------------------------+---------------------------+
| **Fixed pages vs Variable**     | Fixed 4KB pages           | Variable-sized pages      |
+---------------------------------+---------------------------+---------------------------+
| **B+Tree vs Hash indexes**      | B+Tree only               | Multiple index types      |
+---------------------------------+---------------------------+---------------------------+

Future Improvements
^^^^^^^^^^^^^^^^^^^

Potential enhancements (not currently implemented):

1. **Transactions**: Add WAL and transaction manager
2. **Query optimizer**: Cost-based query planning
3. **Concurrency**: MVCC or locking-based concurrency
4. **Compression**: Page-level compression
5. **Additional indexes**: Hash indexes, full-text search
6. **Statistics**: Column histograms for better planning
7. **Caching**: Smarter page replacement (LRU, Clock)
8. **Logging**: Structured logging for debugging

Next Steps
----------

To dive deeper into the architecture:

1. **Storage Layer**: :doc:`storage-engine` - How data is persisted to disk
2. **Index Layer**: :doc:`btree-implementation` - B+Tree index structure and algorithms
3. **Query Layer**: :doc:`expression-templates` - How queries are built at compile-time
4. **Reflection**: :doc:`reflection-system` - Compile-time metadata generation
5. **Performance**: :doc:`performance` - Benchmarks and optimization strategies

References
----------

Academic Papers
^^^^^^^^^^^^^^^

- Bayer, R., & McCreight, E. (1972). "Organization and maintenance of large ordered indices"
- Comer, D. (1979). "The Ubiquitous B-Tree"
- Knuth, D. E. (1998). "The Art of Computer Programming, Volume 3: Sorting and Searching"

Books
^^^^^

- Hellerstein, J. M., & Stonebraker, M. (2007). "Readings in Database Systems" (Red Book)
- Silberschatz, A., Korth, H. F., & Sudarshan, S. (2019). "Database System Concepts"
- Garcia-Molina, H., Ullman, J. D., & Widom, J. (2008). "Database Systems: The Complete Book"

C++ Resources
^^^^^^^^^^^^^

- Vandevoorde, D., Josuttis, N. M., & Gregor, D. (2017). "C++ Templates: The Complete Guide"
- Meyers, S. (2014). "Effective Modern C++"
- Stroustrup, B. (2022). "A Tour of C++ (Third Edition)"

See Also
--------

- :doc:`/getting-started/quick-start` - Get started with LearnQL
- :doc:`/tutorials/tutorial-01-first-database` - Build your first database
- :doc:`/api/index` - Complete API reference
- :doc:`/getting-started/core-concepts` - Core concepts and terminology
