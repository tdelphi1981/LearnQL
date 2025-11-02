Storage Engine
==============

.. contents:: Table of Contents
   :local:
   :depth: 3

Introduction
------------

The storage engine is the foundation of LearnQL, responsible for **persisting data to disk** and managing **page-based storage**. This document explains how LearnQL stores records in fixed-size pages, manages free space, and ensures data durability.

**Key Concepts**:

- **Pages**: Fixed 4KB blocks of storage
- **Free List**: Linked list of deallocated pages for reuse
- **Page Cache**: In-memory cache to reduce disk I/O
- **Durability**: Checksums and flush operations

Why Page-Based Storage?
------------------------

Modern databases use **page-based storage** for several reasons:

1. **OS Alignment**: File systems read/write in blocks (typically 4KB). Using matching page size minimizes I/O operations.

2. **Fixed-Size Allocation**: Simplifies memory management - no need to track variable-sized allocations.

3. **Cache-Friendly**: CPU caches work best with predictable memory access patterns.

4. **Standard Practice**: PostgreSQL (8KB), MySQL/InnoDB (16KB), SQLite (1KB-64KB), Oracle (8KB) all use pages.

LearnQL uses **4KB pages** as a balance between space efficiency and I/O performance.

Page Structure
--------------

Page Layout Overview
^^^^^^^^^^^^^^^^^^^^

Every page in LearnQL has a fixed structure:

.. code-block:: text

   ┌─────────────────────────────────────────────────────────┐
   │                    PAGE (4096 bytes)                     │
   ├─────────────────────────────────────────────────────────┤
   │                                                          │
   │  Page Header (64 bytes)                                 │
   │  ┌──────────────────────────────────────────────────┐   │
   │  │ Magic Number | Page ID | Type | Version | ...    │   │
   │  └──────────────────────────────────────────────────┘   │
   │                                                          │
   ├─────────────────────────────────────────────────────────┤
   │                                                          │
   │                                                          │
   │              Data Section (4032 bytes)                   │
   │                                                          │
   │  [Record 1] [Record 2] [Record 3] ...                   │
   │                                                          │
   │                                           [Free Space]   │
   │                                                          │
   └─────────────────────────────────────────────────────────┘

Page Header Structure
^^^^^^^^^^^^^^^^^^^^^

The 64-byte header contains metadata about the page:

.. code-block:: text

   Offset   Size    Type              Description
   ───────────────────────────────────────────────────────────────
   0-3      4 B     char[4]           Magic: "LQL1" (validation)
   4-11     8 B     uint64_t          Page ID (unique identifier)
   12       1 B     PageType (enum)   Type: FREE, DATA, INDEX, etc.
   13       1 B     uint8_t           Version (format version = 1)
   14-15    2 B     uint16_t          Record count in this page
   16-17    2 B     uint16_t          Free space offset
   18-25    8 B     uint64_t          Next page ID (for linked lists)
   26-29    4 B     uint32_t          Checksum (CRC32)
   30-63    34 B    uint8_t[34]       Reserved for future use

**Field Details**:

- **Magic Number**: ``{'L', 'Q', 'L', '1'}`` - validates page is from LearnQL database
- **Page ID**: Unique identifier, also determines physical offset (``page_id * 4096``)
- **Page Type**: Enum indicating purpose (see below)
- **Version**: Format version for compatibility checks
- **Record Count**: Number of records stored in data section
- **Free Space Offset**: Where free space begins (grows from offset 64)
- **Next Page ID**: For chaining pages (free list, overflow pages)
- **Checksum**: Simple XOR checksum for corruption detection
- **Reserved**: Future enhancements (e.g., timestamps, compression flags)

Page Types
^^^^^^^^^^

LearnQL defines several page types:

.. code-block:: cpp

   enum class PageType : uint8_t {
       FREE = 0,           // Unused page (in free list)
       DATA = 1,           // Contains record data
       INDEX = 2,          // Contains B+tree index nodes
       METADATA = 3,       // Database metadata (page 0 only)
       OVERFLOW_DATA = 4   // Large record overflow data
   };

**Usage**:

- **FREE**: Pages in the free list waiting to be reused
- **DATA**: Regular table data pages
- **INDEX**: B+tree index pages (see :doc:`btree-implementation`)
- **METADATA**: Special page 0 with database-level metadata
- **OVERFLOW_DATA**: For records larger than one page (future enhancement)

Page Implementation
^^^^^^^^^^^^^^^^^^^

From ``learnql/storage/Page.hpp``:

.. code-block:: cpp

   class Page {
   public:
       static constexpr std::size_t DATA_SIZE = PAGE_SIZE - sizeof(PageHeader);

       // Constructor
       explicit Page(uint64_t page_id, PageType type = PageType::DATA) noexcept
           : header_{}, data_{} {
           header_.page_id = page_id;
           header_.page_type = type;
       }

       // Access header and data
       PageHeader& header() noexcept;
       std::span<uint8_t> data() noexcept;

       // Write data to page
       void write_data(std::size_t offset, const void* src, std::size_t size);

       // Read data from page
       void read_data(std::size_t offset, void* dest, std::size_t size) const;

       // Calculate free space
       std::size_t available_space() const noexcept {
           return DATA_SIZE - header_.free_space_offset + sizeof(PageHeader);
       }

       // Checksum operations
       uint32_t compute_checksum() const noexcept;
       void update_checksum() noexcept;
       bool validate_checksum() const noexcept;

   private:
       PageHeader header_;                      // 64 bytes
       std::array<uint8_t, DATA_SIZE> data_;    // 4032 bytes
   };

   static_assert(sizeof(Page) == PAGE_SIZE);  // Compile-time check

**Design Notes**:

- Uses ``std::array`` for fixed-size allocation (no dynamic memory)
- ``std::span`` provides safe, bounds-checked access to data
- Compile-time assertion ensures correct size
- C++20 three-way comparison (``operator<=>``) for header comparisons

Storage Engine Architecture
---------------------------

High-Level Design
^^^^^^^^^^^^^^^^^

.. code-block:: text

   ┌────────────────────────────────────────────────────────┐
   │               StorageEngine                            │
   ├────────────────────────────────────────────────────────┤
   │                                                        │
   │  ┌──────────────┐  ┌──────────────┐  ┌────────────┐  │
   │  │ Page Manager │  │ Page Cache   │  │ Free List  │  │
   │  │              │  │              │  │            │  │
   │  │ - Allocate   │  │ - LRU evict  │  │ - Reuse    │  │
   │  │ - Deallocate │  │ - Dirty      │  │   pages    │  │
   │  │ - Read/Write │  │   tracking   │  │            │  │
   │  └──────────────┘  └──────────────┘  └────────────┘  │
   │                                                        │
   │  ┌────────────────────────────────────────────────┐   │
   │  │         Metadata Manager                        │   │
   │  │  - sys_tables_root                              │   │
   │  │  - sys_fields_root                              │   │
   │  │  - sys_indexes_root                             │   │
   │  └────────────────────────────────────────────────┘   │
   │                                                        │
   └────────────────────────┬───────────────────────────────┘
                            │
                            ▼
                    ┌──────────────┐
                    │ database.db  │
                    │ (file)       │
                    └──────────────┘

Core Operations
^^^^^^^^^^^^^^^

**1. Page Allocation**

Algorithm for ``allocate_page()``:

.. code-block:: cpp

   uint64_t allocate_page(PageType type) {
       uint64_t page_id;

       // Try to reuse a free page
       if (free_list_head_ != 0) {
           page_id = free_list_head_;
           Page page = read_page(page_id);
           free_list_head_ = page.header().next_page_id;  // Pop from free list

           // Reset the page
           page = Page(page_id, type);
           write_page(page_id, page);
       } else {
           // Allocate a new page at end of file
           page_id = next_page_id_++;
           Page page(page_id, type);
           write_page(page_id, page);
       }

       save_metadata();  // Persist metadata changes
       return page_id;
   }

**Flow**:

1. Check if free list has available pages
2. If yes, reuse the head of free list
3. If no, allocate new page at end of file
4. Initialize page with correct type
5. Update metadata

**Time Complexity**: O(1)

**2. Page Deallocation**

Algorithm for ``deallocate_page()``:

.. code-block:: cpp

   void deallocate_page(uint64_t page_id) {
       if (page_id == 0) {
           throw std::invalid_argument("Cannot deallocate metadata page");
       }

       // Read the page and mark it as free
       Page page = read_page(page_id);
       page.header().page_type = PageType::FREE;
       page.header().next_page_id = free_list_head_;  // Link to current head
       page.clear();  // Zero out data

       // Add to head of free list
       free_list_head_ = page_id;
       write_page(page_id, page);

       save_metadata();
   }

**Flow**:

1. Validate page ID (can't deallocate page 0)
2. Mark page as FREE type
3. Link page to current free list head
4. Update free list head to this page
5. Clear page data (security/debugging)

**Time Complexity**: O(1)

**3. Page Reading**

Algorithm for ``read_page()`` with caching:

.. code-block:: cpp

   Page read_page(uint64_t page_id) {
       // Check cache first
       auto it = page_cache_.find(page_id);
       if (it != page_cache_.end()) {
           return it->second;  // Cache hit
       }

       // Cache miss - read from disk
       std::ifstream file(file_path_, std::ios::binary);
       file.seekg(page_id * PAGE_SIZE);  // Seek to page offset

       Page page;
       file.read(static_cast<char*>(page.raw_data()), PAGE_SIZE);

       // Validate page
       if (!page.header().is_valid()) {
           throw std::runtime_error("Invalid page header");
       }

       // Add to cache (evict if full)
       if (page_cache_.size() >= cache_size_) {
           evict_page();
       }
       page_cache_[page_id] = page;

       return page;
   }

**Flow**:

1. Check cache for page (O(1) hash lookup)
2. If cached, return immediately
3. If not cached, read from file at offset ``page_id * 4096``
4. Validate magic number and checksum
5. Add to cache (evict if necessary)

**Time Complexity**:
- Cache hit: O(1)
- Cache miss: O(1) + disk I/O

**4. Page Writing**

Algorithm for ``write_page()`` with dirty tracking:

.. code-block:: cpp

   void write_page(uint64_t page_id, const Page& page) {
       // Update cache
       page_cache_[page_id] = page;
       dirty_pages_.insert(page_id);  // Mark as dirty

       // If too many dirty pages, flush to disk
       if (dirty_pages_.size() > cache_size_ / 2) {
           flush_all();
       }
   }

**Lazy Writing**: Pages are not immediately written to disk. Instead, they're marked "dirty" and flushed later in batches. This improves performance by reducing I/O operations.

**5. Flushing**

Algorithm for ``flush_all()``:

.. code-block:: cpp

   void flush_all() {
       if (dirty_pages_.empty()) return;

       std::fstream file(file_path_, std::ios::binary | std::ios::in | std::ios::out);

       for (uint64_t page_id : dirty_pages_) {
           auto it = page_cache_.find(page_id);
           if (it != page_cache_.end()) {
               Page page = it->second;
               page.update_checksum();  // Compute checksum before writing

               file.seekp(page_id * PAGE_SIZE);
               file.write(static_cast<const char*>(page.raw_data()), PAGE_SIZE);

               page_cache_[page_id] = page;  // Update cache with checksummed page
           }
       }

       file.flush();  // Force OS to write to disk
       dirty_pages_.clear();
   }

**Flow**:

1. Open file in read/write mode
2. For each dirty page:
   - Compute checksum
   - Seek to page offset
   - Write 4096 bytes
3. Call ``file.flush()`` to ensure OS writes data
4. Clear dirty tracking set

**Time Complexity**: O(d) where d = number of dirty pages

Free Space Management
---------------------

Free List Algorithm
^^^^^^^^^^^^^^^^^^^

LearnQL uses a **linked list** of free pages:

.. code-block:: text

   free_list_head = 7

   Page 7 (FREE)          Page 12 (FREE)        Page 5 (FREE)
   ┌────────────┐         ┌────────────┐        ┌────────────┐
   │ next = 12  │────────>│ next = 5   │───────>│ next = 0   │
   └────────────┘         └────────────┘        └────────────┘
                                                (end of list)

**Operations**:

.. code-block:: text

   Allocate:
   1. Pop head: page_id = 7
   2. Update head: free_list_head = 12
   3. Return page_id = 7

   Deallocate (page 3):
   1. Set page 3's next = 7 (current head)
   2. Update head: free_list_head = 3
   3. Page 3 is now at front of free list

**Advantages**:

- Simple to implement
- O(1) allocation and deallocation
- No fragmentation issues (fixed-size pages)

**Disadvantages**:

- No locality awareness (reused pages may be scattered)
- No sorting by page ID

**Future Enhancement**: Bitmap-based free space tracking for better locality.

Page Cache Management
^^^^^^^^^^^^^^^^^^^^^

The page cache is a simple **hash map** with **LRU-like eviction**:

.. code-block:: cpp

   std::unordered_map<uint64_t, Page> page_cache_;
   std::unordered_set<uint64_t> dirty_pages_;
   std::size_t cache_size_;  // Default: 64 pages = 256 KB

**Eviction Policy** (``evict_page()``):

.. code-block:: cpp

   void evict_page() {
       // Find first non-dirty page
       for (auto it = page_cache_.begin(); it != page_cache_.end(); ++it) {
           if (dirty_pages_.find(it->first) == dirty_pages_.end()) {
               page_cache_.erase(it);
               return;
           }
       }

       // If all pages are dirty, flush and evict first
       uint64_t page_id = page_cache_.begin()->first;
       flush_page(page_id);
       page_cache_.erase(page_id);
   }

**Strategy**:

1. Prefer to evict clean (non-dirty) pages (no disk I/O needed)
2. If all pages are dirty, flush the first one and evict
3. Not true LRU (would need access timestamp tracking)

**Trade-offs**:

- **Pro**: Simple and fast
- **Con**: Not optimal for all workloads
- **Better alternative**: Clock algorithm, true LRU with doubly-linked list

Record Storage Format
---------------------

How Records Are Stored
^^^^^^^^^^^^^^^^^^^^^^^

Records are serialized into pages using the ``BinaryWriter``:

.. code-block:: text

   Page Data Section:

   Offset  Content
   ─────────────────────────────────────────────────────────
   0       Record 1 Length (4 bytes, uint32_t)
   4       Record 1 Data (variable length, serialized)

   ...     Record 2 Length (4 bytes)
           Record 2 Data

   ...     Record 3 Length (4 bytes)
           Record 3 Data

   ...     (Free Space)

**Serialization Example** (Student record):

.. code-block:: cpp

   class Student {
       int student_id_;           // 4 bytes
       std::string name_;         // 4 bytes (length) + N bytes (data)
       int age_;                  // 4 bytes
       double gpa_;               // 8 bytes
   };

Serialized format:

.. code-block:: text

   [Total Length: 4 bytes]
   [student_id: 4 bytes]
   [name length: 4 bytes][name data: N bytes]
   [age: 4 bytes]
   [gpa: 8 bytes]

**Example**:

.. code-block:: text

   Student(42, "Alice", 20, 3.8)

   Bytes: [29] [42] [5] ['A']['l']['i']['c']['e'] [20] [3.8]
          └─┘  └─┘  └─┘  └─────────────────────┘  └─┘  └─┘
         Length ID NameLen      Name              Age  GPA

Record Spans Multiple Pages
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Current Limitation**: Records must fit in a single page (< 4032 bytes).

**Future Enhancement**: Overflow pages for large records

.. code-block:: text

   Large Record Storage (Not Yet Implemented):

   Page 10 (DATA)                Page 25 (OVERFLOW_DATA)
   ┌─────────────────┐          ┌─────────────────┐
   │ Record Prefix   │          │ Record Suffix   │
   │ overflow=25 ────┼─────────>│ next=0          │
   │ [partial data]  │          │ [more data]     │
   └─────────────────┘          └─────────────────┘

**Algorithm**:

1. Write as much as fits in first page
2. Set ``overflow`` pointer to next page
3. Continue writing in overflow page
4. Chain overflow pages if needed

File Format Specification
-------------------------

Complete Database File Structure
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: text

   database.db

   ┌──────────────────────────────────────────────────────┐
   │ Page 0: METADATA PAGE                                │
   ├──────────────────────────────────────────────────────┤
   │                                                      │
   │  Magic: "LearnQL Database" (16 bytes)               │
   │  next_page_id: 42          (8 bytes)                │
   │  free_list_head: 7         (8 bytes)                │
   │  sys_tables_root: 1        (8 bytes)                │
   │  sys_fields_root: 2        (8 bytes)                │
   │  version: 3                (4 bytes)                │
   │  created_timestamp: ...    (8 bytes)                │
   │  sys_indexes_root: 3       (8 bytes)                │
   │  reserved: ...             (60 bytes)               │
   │                                                      │
   └──────────────────────────────────────────────────────┘
   ┌──────────────────────────────────────────────────────┐
   │ Page 1: INDEX PAGE (sys_tables B+tree root)         │
   └──────────────────────────────────────────────────────┘
   ┌──────────────────────────────────────────────────────┐
   │ Page 2: INDEX PAGE (sys_fields B+tree root)         │
   └──────────────────────────────────────────────────────┘
   ┌──────────────────────────────────────────────────────┐
   │ Page 3: INDEX PAGE (sys_indexes B+tree root)        │
   └──────────────────────────────────────────────────────┘
   ┌──────────────────────────────────────────────────────┐
   │ Page 4: DATA PAGE (students table)                  │
   └──────────────────────────────────────────────────────┘
   ┌──────────────────────────────────────────────────────┐
   │ Page 5: DATA PAGE (students table)                  │
   └──────────────────────────────────────────────────────┘
   ┌──────────────────────────────────────────────────────┐
   │ Page 6: INDEX PAGE (students primary key index)     │
   └──────────────────────────────────────────────────────┘
   ┌──────────────────────────────────────────────────────┐
   │ Page 7: FREE PAGE (in free list)                    │
   └──────────────────────────────────────────────────────┘
   ...

Version Compatibility
^^^^^^^^^^^^^^^^^^^^^

LearnQL supports database versions 2 and 3:

**Version 2** (Legacy):

- No ``sys_indexes_root`` field
- Secondary indexes not supported

**Version 3** (Current):

- Added ``sys_indexes_root`` at offset 60
- Full secondary index support

**Loading Logic**:

.. code-block:: cpp

   void load_metadata() {
       Page metadata_page = read_page(0);

       uint32_t version;
       metadata_page.read_data(48, &version, sizeof(version));

       if (version == 2) {
           sys_indexes_root_ = 0;  // Not present in v2
       } else if (version == 3) {
           metadata_page.read_data(60, &sys_indexes_root_, sizeof(sys_indexes_root_));
       } else {
           throw std::runtime_error("Unsupported database version");
       }
   }

Durability and Consistency
---------------------------

Crash Recovery
^^^^^^^^^^^^^^

**Current Behavior**: LearnQL is NOT crash-safe.

**Issues**:

1. **Torn Pages**: If crash occurs during page write, page may be partially written
2. **Lost Updates**: Dirty pages not flushed are lost
3. **No Undo/Redo**: Cannot rollback or replay operations

**Mitigation**:

- Call ``flush_all()`` at safe points to ensure durability
- Use checksums to detect corruption

**Future Enhancement**: Write-Ahead Logging (WAL)

.. code-block:: text

   With WAL:

   1. Write operation log entry to WAL file
   2. Flush WAL entry (small, fast write)
   3. Modify page in cache (mark dirty)
   4. Periodically checkpoint: flush dirty pages

   On crash recovery:
   - Replay WAL from last checkpoint
   - Ensures ACID durability

Checksum Validation
^^^^^^^^^^^^^^^^^^^

Simple XOR checksum for corruption detection:

.. code-block:: cpp

   uint32_t compute_checksum() const noexcept {
       uint32_t checksum = 0;
       for (const auto& byte : data_) {
           checksum ^= byte;
       }
       return checksum;
   }

**Limitations**:

- Weak checksum (doesn't detect all corruption)
- No error correction (only detection)

**Better Alternative**: CRC32 or CRC64 for production use

Code Examples
-------------

Example 1: Allocating and Writing a Page
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   #include <learnql/storage/StorageEngine.hpp>

   using namespace learnql::storage;

   int main() {
       // Create storage engine
       StorageEngine storage("example.db");

       // Allocate a new data page
       uint64_t page_id = storage.allocate_page(PageType::DATA);

       // Write some data to the page
       Page page = storage.read_page(page_id);

       std::string data = "Hello, LearnQL!";
       page.write_data(0, data.data(), data.size());

       // Update and write back
       storage.write_page(page_id, page);
       storage.flush_all();  // Ensure durability

       return 0;
   }

Example 2: Reading from Free List
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // First allocation (no free pages, allocates page 1)
   uint64_t page1 = storage.allocate_page(PageType::DATA);
   std::cout << "Allocated page: " << page1 << "\n";  // Output: 1

   // Second allocation (allocates page 2)
   uint64_t page2 = storage.allocate_page(PageType::DATA);
   std::cout << "Allocated page: " << page2 << "\n";  // Output: 2

   // Deallocate page 1 (adds to free list)
   storage.deallocate_page(page1);

   // Third allocation (reuses page 1 from free list!)
   uint64_t page3 = storage.allocate_page(PageType::DATA);
   std::cout << "Allocated page: " << page3 << "\n";  // Output: 1 (reused!)

Example 3: Page Cache Behavior
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // Configure small cache for demonstration
   StorageEngine storage("example.db", /*cache_size=*/ 3);

   // Read 3 pages (all cached)
   Page p1 = storage.read_page(1);  // Cache miss, reads from disk
   Page p2 = storage.read_page(2);  // Cache miss, reads from disk
   Page p3 = storage.read_page(3);  // Cache miss, reads from disk

   // Read page 1 again (cache hit!)
   Page p1_again = storage.read_page(1);  // Cache hit, no disk I/O

   // Read page 4 (cache full, evicts a page)
   Page p4 = storage.read_page(4);  // Cache miss, evicts clean page

Performance Analysis
--------------------

Time Complexity Summary
^^^^^^^^^^^^^^^^^^^^^^^

+------------------------+------------------+------------------------+
| Operation              | Time Complexity  | Notes                  |
+========================+==================+========================+
| ``allocate_page()``    | O(1)             | Constant time          |
+------------------------+------------------+------------------------+
| ``deallocate_page()``  | O(1)             | Constant time          |
+------------------------+------------------+------------------------+
| ``read_page()``        | O(1) + I/O       | Cache hit is O(1)      |
+------------------------+------------------+------------------------+
| ``write_page()``       | O(1)             | Lazy write to cache    |
+------------------------+------------------+------------------------+
| ``flush_all()``        | O(d)             | d = dirty pages        |
+------------------------+------------------+------------------------+

Space Complexity
^^^^^^^^^^^^^^^^

- **Page overhead**: 64 bytes per page (1.56% overhead)
- **Cache memory**: ``cache_size * 4096`` bytes (default: 256 KB)
- **Metadata**: Fixed ~100 bytes

Optimization Strategies
^^^^^^^^^^^^^^^^^^^^^^^

1. **Increase cache size** for workloads with hot pages

   .. code-block:: cpp

      StorageEngine storage("data.db", /*cache_size=*/ 256);  // 1 MB cache

2. **Batch operations** to reduce flush calls

   .. code-block:: cpp

      for (int i = 0; i < 1000; ++i) {
           storage.write_page(...);  // All cached
       }
       storage.flush_all();  // Single flush at end

3. **Pre-allocate pages** if you know data size

   .. code-block:: cpp

      std::vector<uint64_t> page_ids;
       for (int i = 0; i < 100; ++i) {
           page_ids.push_back(storage.allocate_page(PageType::DATA));
       }

Design Trade-offs
-----------------

+---------------------------------+---------------------------+---------------------------+
| Decision                        | Chosen Approach           | Alternative               |
+=================================+===========================+===========================+
| **Page size**                   | Fixed 4KB                 | Variable or 8KB/16KB      |
+---------------------------------+---------------------------+---------------------------+
| **Free space management**       | Linked free list          | Bitmap, buddy system      |
+---------------------------------+---------------------------+---------------------------+
| **Cache eviction**              | Simple LRU-like           | Clock, LRU-K, ARC         |
+---------------------------------+---------------------------+---------------------------+
| **Checksum algorithm**          | XOR (fast, weak)          | CRC32 (slower, stronger)  |
+---------------------------------+---------------------------+---------------------------+
| **Page allocation**             | End-of-file growth        | Pre-allocation pools      |
+---------------------------------+---------------------------+---------------------------+
| **Durability**                  | Manual flush              | WAL, shadow paging        |
+---------------------------------+---------------------------+---------------------------+

Limitations
-----------

1. **No ACID transactions**: Cannot rollback partial operations
2. **No concurrency control**: Single-threaded access only
3. **Weak checksums**: XOR checksums don't detect all corruption
4. **No compression**: Pages stored as-is (future: page-level compression)
5. **Fixed page size**: Cannot adjust per workload
6. **No statistics**: No histograms or page fill factor tracking

Future Improvements
-------------------

1. **Write-Ahead Logging (WAL)**: For crash recovery and ACID durability
2. **Better checksums**: CRC32 or CRC64
3. **Compression**: Snappy or LZ4 page compression
4. **Smarter cache**: True LRU with access frequency tracking
5. **Page fill factor**: Track and report page utilization
6. **Async I/O**: Use ``io_uring`` (Linux) or ``IOCP`` (Windows)
7. **Mmap support**: Memory-mapped file access for large pages
8. **Direct I/O**: Bypass OS page cache for database cache control

See Also
--------

- :doc:`btree-implementation` - How indexes use storage pages
- :doc:`performance` - Benchmarks and profiling
- :doc:`overview` - Overall architecture

References
----------

- Gray, J., & Reuter, A. (1992). "Transaction Processing: Concepts and Techniques"
- Hellerstein, J. M., Stonebraker, M., & Hamilton, J. (2007). "Architecture of a Database System"
- SQLite file format documentation: https://www.sqlite.org/fileformat.html
- PostgreSQL internals: https://www.postgresql.org/docs/current/storage.html
