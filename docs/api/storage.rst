Storage Module
==============

.. contents:: Table of Contents
   :local:
   :depth: 2

Overview
--------

The Storage module implements LearnQL's page-based storage architecture. It provides low-level disk I/O, page management, caching, and free space tracking. While most users won't interact directly with this module, understanding it helps in performance tuning and troubleshooting.

**Key Features:**

- Fixed-size page-based storage (4KB pages)
- Page caching for performance
- Free list management for deleted pages
- ACID-like flush operations
- Metadata persistence
- Checksum validation

**Module Components:**

- ``StorageEngine`` - Main storage management class
- ``Page`` - Fixed-size storage unit (4KB)
- ``PageHeader`` - Page metadata structure
- ``PageType`` - Enumeration of page types

.. note::
   This module is primarily for educational purposes and internal use. Most applications should use the high-level ``Database`` and ``Table`` APIs instead.

Architecture
------------

Page-Based Storage
~~~~~~~~~~~~~~~~~~

LearnQL uses a page-based architecture similar to traditional database systems:

.. code-block:: text

   Database File Layout
   ┌─────────────────────────────────────┐
   │ Page 0: Metadata Page               │  ← Database info, free list
   ├─────────────────────────────────────┤
   │ Page 1: Data Page (e.g., students)  │
   ├─────────────────────────────────────┤
   │ Page 2: Index Page (B-tree node)    │
   ├─────────────────────────────────────┤
   │ Page 3: Data Page                   │
   ├─────────────────────────────────────┤
   │ Page 4: FREE (deleted, reusable)    │
   ├─────────────────────────────────────┤
   │ ...                                 │
   └─────────────────────────────────────┘

   Each Page = 4096 bytes (4KB)

Page Structure
~~~~~~~~~~~~~~

Each page consists of a header and data area:

.. code-block:: text

   Page Layout (4096 bytes total)
   ┌──────────────────────────────────────┐
   │ PageHeader (64 bytes)                │
   │  - Magic number (4 bytes)            │
   │  - Page ID (8 bytes)                 │
   │  - Page type (4 bytes)               │
   │  - Free space offset (4 bytes)       │
   │  - Record count (4 bytes)            │
   │  - Next page ID (8 bytes)            │
   │  - Checksum (4 bytes)                │
   │  - Reserved (28 bytes)               │
   ├──────────────────────────────────────┤
   │ Data Area (4032 bytes)               │
   │  - Variable-length records           │
   │  - Free space                        │
   └──────────────────────────────────────┘

Page Types
~~~~~~~~~~

.. code-block:: cpp

   enum class PageType : uint32_t {
       FREE = 0,      // Unused page (in free list)
       DATA = 1,      // Data records
       INDEX = 2,     // B-tree index node
       METADATA = 3   // Database metadata (page 0)
   };

Class Reference
---------------

StorageEngine Class
~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: learnql::storage::StorageEngine
   :members:
   :undoc-members:

The ``StorageEngine`` class manages all disk I/O operations, page allocation, and caching.

**Constructor:**

.. code-block:: cpp

   explicit StorageEngine(
       const std::string& file_path,
       std::size_t cache_size = 64
   );

**Parameters:**

- ``file_path`` - Path to database file (created if doesn't exist)
- ``cache_size`` - Number of pages to cache in memory (default: 64)

**Throws:** ``std::runtime_error`` if file cannot be opened

**Example:**

.. code-block:: cpp

   #include <learnql/storage/StorageEngine.hpp>

   // Create or open a database file
   auto storage = std::make_shared<StorageEngine>("myapp.db", 128);

Page Allocation
^^^^^^^^^^^^^^^

``allocate_page()`` - Allocate New Page
""""""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   [[nodiscard]] uint64_t allocate_page(PageType type = PageType::DATA);

Allocates a new page, reusing from free list if available.

**Parameters:**

- ``type`` - Type of page to allocate

**Returns:** Page ID of the allocated page

**Example:**

.. code-block:: cpp

   uint64_t data_page_id = storage->allocate_page(PageType::DATA);
   uint64_t index_page_id = storage->allocate_page(PageType::INDEX);

``deallocate_page()`` - Free Page
""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   void deallocate_page(uint64_t page_id);

Deallocates a page and adds it to the free list for reuse.

**Parameters:**

- ``page_id`` - ID of page to deallocate

**Throws:** ``std::invalid_argument`` if attempting to deallocate page 0 (metadata)

**Example:**

.. code-block:: cpp

   storage->deallocate_page(old_page_id);

Page I/O
^^^^^^^^

``read_page()`` - Read Page from Disk
""""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   [[nodiscard]] Page read_page(uint64_t page_id);

Reads a page from storage, using cache if available.

**Parameters:**

- ``page_id`` - ID of page to read

**Returns:** The page object

**Throws:** ``std::runtime_error`` if page cannot be read or is invalid

**Example:**

.. code-block:: cpp

   Page page = storage->read_page(page_id);

   // Access page data
   uint32_t record_count = page.header().record_count;

``write_page()`` - Write Page to Cache
"""""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   void write_page(uint64_t page_id, const Page& page);

Writes a page to the cache (marked as dirty). Actual disk write occurs during flush.

**Parameters:**

- ``page_id`` - ID of the page
- ``page`` - Page object to write

**Example:**

.. code-block:: cpp

   Page page(page_id, PageType::DATA);
   // ... modify page ...
   storage->write_page(page_id, page);

Flushing
^^^^^^^^

``flush_all()`` - Flush All Dirty Pages
""""""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   void flush_all();

Writes all dirty pages to disk and clears the dirty page set.

**Example:**

.. code-block:: cpp

   // Make changes
   storage->write_page(page_id, modified_page);

   // Ensure changes are persisted
   storage->flush_all();

``flush_page()`` - Flush Single Page
"""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   void flush_page(uint64_t page_id);

Writes a specific dirty page to disk.

**Parameters:**

- ``page_id`` - ID of page to flush

Metadata Management
^^^^^^^^^^^^^^^^^^^

``get_sys_tables_root()`` - Get System Tables Root
"""""""""""""""""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   [[nodiscard]] uint64_t get_sys_tables_root() const;

Returns the root page ID for the ``_sys_tables`` catalog table.

``set_sys_tables_root()`` - Set System Tables Root
"""""""""""""""""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   void set_sys_tables_root(uint64_t root_page_id);

Sets the root page ID for the ``_sys_tables`` catalog table.

``get_sys_fields_root()`` / ``set_sys_fields_root()``
""""""""""""""""""""""""""""""""""""""""""""""""""""""

Similar to above, but for ``_sys_fields`` catalog table.

``get_sys_indexes_root()`` / ``set_sys_indexes_root()``
""""""""""""""""""""""""""""""""""""""""""""""""""""""""

Similar to above, but for ``_sys_indexes`` catalog table (version 3+ databases).

Utility Methods
^^^^^^^^^^^^^^^

``get_page_count()`` - Get Total Pages
"""""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   [[nodiscard]] uint64_t get_page_count() const noexcept;

Returns the total number of pages allocated (including free pages).

``clear_cache()`` - Clear Page Cache
"""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   void clear_cache();

Flushes all dirty pages and clears the cache. Useful for testing or forcing disk writes.

Page Class
~~~~~~~~~~

.. doxygenclass:: learnql::storage::Page
   :members:

The ``Page`` class represents a single 4KB storage page.

**Constructor:**

.. code-block:: cpp

   Page(uint64_t page_id = 0, PageType type = PageType::DATA);

**Methods:**

``header()`` - Access Page Header
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   PageHeader& header() noexcept;
   const PageHeader& header() const noexcept;

Returns reference to the page header.

``write_data()`` - Write Data to Page
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   void write_data(std::size_t offset, const void* data, std::size_t size);

Writes data to the page at the specified offset.

**Parameters:**

- ``offset`` - Byte offset within page (0-4095)
- ``data`` - Pointer to data to write
- ``size`` - Number of bytes to write

**Example:**

.. code-block:: cpp

   int value = 42;
   page.write_data(100, &value, sizeof(value));

``read_data()`` - Read Data from Page
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   void read_data(std::size_t offset, void* buffer, std::size_t size) const;

Reads data from the page at the specified offset.

``update_checksum()`` - Compute Checksum
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   void update_checksum();

Computes and updates the page checksum. Called automatically before writing to disk.

``clear()`` - Clear Page Data
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   void clear();

Clears the page data area (header is preserved).

PageHeader Structure
~~~~~~~~~~~~~~~~~~~~

.. doxygenstruct:: learnql::storage::PageHeader
   :members:

The page header contains metadata about the page.

**Fields:**

.. code-block:: cpp

   struct PageHeader {
       uint32_t magic;              // Magic number (0xDEADBEEF)
       uint64_t page_id;            // Page identifier
       PageType page_type;          // Type of page
       uint32_t free_space_offset;  // Offset to free space
       uint32_t record_count;       // Number of records
       uint64_t next_page_id;       // Next page (for chaining)
       uint32_t checksum;           // Page checksum
       uint8_t reserved[28];        // Reserved for future use
   };

Database File Format
--------------------

Metadata Page (Page 0)
~~~~~~~~~~~~~~~~~~~~~~

The first page in every LearnQL database contains metadata:

.. code-block:: text

   Offset  Size  Field
   ──────────────────────────────────────
   0-15    16    "LearnQL Database" header
   16-23   8     next_page_id
   24-31   8     free_list_head
   32-39   8     sys_tables_root
   40-47   8     sys_fields_root
   48-51   4     database version
   52-59   8     created_timestamp
   60-67   8     sys_indexes_root (v3+)

**Database Versions:**

- Version 2: Basic tables and fields, no secondary indexes
- Version 3: Adds secondary index support

Free List Management
~~~~~~~~~~~~~~~~~~~~

Deleted pages are organized in a linked list:

.. code-block:: text

   Metadata Page
   free_list_head = 5
         ↓
   Page 5 (FREE)
   next_page_id = 12
         ↓
   Page 12 (FREE)
   next_page_id = 0 (end)

When a page is deallocated:

1. Set page type to ``FREE``
2. Set ``next_page_id`` to current ``free_list_head``
3. Update ``free_list_head`` to point to this page

When allocating a page:

1. If ``free_list_head != 0``, reuse that page
2. Otherwise, allocate new page with ``next_page_id++``

Usage Examples
--------------

Creating a Storage Engine
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <learnql/storage/StorageEngine.hpp>

   // Create new database or open existing
   auto storage = std::make_shared<StorageEngine>(
       "myapp.db",
       128  // Cache up to 128 pages (512KB)
   );

   std::cout << "Total pages: " << storage->get_page_count() << "\n";

Allocating and Using Pages
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Allocate a data page
   uint64_t page_id = storage->allocate_page(PageType::DATA);

   // Read the page
   Page page = storage->read_page(page_id);

   // Write some data
   std::string data = "Hello, LearnQL!";
   page.write_data(0, data.c_str(), data.size());
   page.header().record_count = 1;

   // Write back to storage
   storage->write_page(page_id, page);

   // Persist to disk
   storage->flush_all();

Reading Page Data
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Read page from storage
   Page page = storage->read_page(page_id);

   // Read data
   std::vector<char> buffer(100);
   page.read_data(0, buffer.data(), buffer.size());

   // Access header information
   std::cout << "Page type: " << static_cast<int>(page.header().page_type) << "\n";
   std::cout << "Records: " << page.header().record_count << "\n";

Page Lifecycle
~~~~~~~~~~~~~~

.. code-block:: cpp

   // 1. Allocate page
   uint64_t page_id = storage->allocate_page(PageType::DATA);

   // 2. Use page for data storage
   Page page = storage->read_page(page_id);
   // ... write data ...
   storage->write_page(page_id, page);

   // 3. When no longer needed, deallocate
   storage->deallocate_page(page_id);

   // 4. Page returns to free list and can be reused
   uint64_t new_page_id = storage->allocate_page(PageType::DATA);
   // new_page_id might equal page_id (page was reused)

Caching Behavior
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   auto storage = std::make_shared<StorageEngine>("test.db", 4);  // Cache 4 pages

   // First read - loads from disk into cache
   auto page1 = storage->read_page(1);  // DISK READ

   // Second read - served from cache
   auto page2 = storage->read_page(1);  // CACHE HIT

   // Reading 5 different pages
   for (uint64_t i = 1; i <= 5; ++i) {
       auto page = storage->read_page(i);  // 5th read evicts oldest page
   }

   // Clear cache (flushes dirty pages first)
   storage->clear_cache();

Performance Considerations
--------------------------

Cache Size
~~~~~~~~~~

The cache size directly impacts performance and memory usage:

.. code-block:: cpp

   // Small cache - low memory, more disk I/O
   StorageEngine small_cache("db.dat", 16);  // 64KB cache

   // Large cache - high memory, less disk I/O
   StorageEngine large_cache("db.dat", 256);  // 1MB cache

**Memory Usage Formula:**

.. code-block:: text

   Memory = cache_size × 4096 bytes

   Examples:
   - 64 pages   = 256 KB
   - 128 pages  = 512 KB
   - 256 pages  = 1 MB
   - 1024 pages = 4 MB

**Recommendations:**

- Interactive applications: 64-128 pages (256KB-512KB)
- Batch processing: 256-512 pages (1MB-2MB)
- Large datasets: 1024+ pages (4MB+)

Write Patterns
~~~~~~~~~~~~~~

.. code-block:: cpp

   // INEFFICIENT - flush after every write
   for (int i = 0; i < 1000; ++i) {
       storage->write_page(i, page);
       storage->flush_all();  // DON'T DO THIS
   }

   // EFFICIENT - batch writes, flush once
   for (int i = 0; i < 1000; ++i) {
       storage->write_page(i, page);
   }
   storage->flush_all();  // Flush at end

**Note:** The storage engine automatically flushes when dirty page count exceeds ``cache_size / 2``.

Free List Fragmentation
~~~~~~~~~~~~~~~~~~~~~~~~

Over time, the free list can become fragmented. Currently, LearnQL doesn't compact the free list, but pages are reused in LIFO order (last freed, first reused).

Checksum Validation
~~~~~~~~~~~~~~~~~~~

Every page read is validated:

.. code-block:: cpp

   Page page = storage->read_page(page_id);
   // If checksum fails, throws std::runtime_error

This provides basic corruption detection. For production use, consider additional backup strategies.

Internal Details
----------------

Page Cache Implementation
~~~~~~~~~~~~~~~~~~~~~~~~~

The storage engine uses an ``std::unordered_map`` for the page cache:

.. code-block:: cpp

   std::unordered_map<uint64_t, Page> page_cache_;      // Page ID → Page
   std::unordered_set<uint64_t> dirty_pages_;           // Dirty page IDs

**Cache Eviction Policy:**

1. Simple LRU-like: evict first non-dirty page
2. If all pages dirty, flush and evict oldest

Flush Operation
~~~~~~~~~~~~~~~

When ``flush_all()`` is called:

1. Iterate all dirty page IDs
2. For each dirty page:

   a. Retrieve from cache
   b. Update checksum
   c. Seek to page offset in file
   d. Write 4096 bytes
   e. Update cache with checksummed page

3. Call ``file.flush()`` to ensure OS writes to disk
4. Clear dirty page set

File I/O
~~~~~~~~

The storage engine uses ``std::fstream`` for I/O:

.. code-block:: cpp

   // Read operation
   std::ifstream file(file_path_, std::ios::binary);
   file.seekg(page_id * PAGE_SIZE);
   file.read(static_cast<char*>(page.raw_data()), PAGE_SIZE);

   // Write operation
   std::fstream file(file_path_, std::ios::binary | std::ios::in | std::ios::out);
   file.seekp(page_id * PAGE_SIZE);
   file.write(static_cast<const char*>(page.raw_data()), PAGE_SIZE);

Limitations
-----------

Current Limitations
~~~~~~~~~~~~~~~~~~~

1. **No Transactions:** Changes are not atomic. A crash during ``flush_all()`` can corrupt the database.
2. **No WAL:** No write-ahead logging for crash recovery.
3. **Single-threaded:** No thread safety or concurrent access support.
4. **No Compression:** Pages are stored uncompressed.
5. **Fixed Page Size:** Cannot change 4KB page size without recreating database.

**Educational Note:** These limitations are intentional for learning purposes. Production databases address these with WAL, MVCC, and other techniques.

Troubleshooting
---------------

Corrupted Database File
~~~~~~~~~~~~~~~~~~~~~~~

**Symptoms:** ``std::runtime_error`` with "Invalid page header" or "Invalid database file format"

**Causes:**

- Application crashed during write
- File was manually edited
- Disk corruption

**Solutions:**

- Restore from backup
- Recreate database (if no backup available)

**Prevention:**

.. code-block:: cpp

   // Always flush before exiting
   storage->flush_all();

   // Or use RAII (destructor flushes automatically)
   {
       auto storage = std::make_shared<StorageEngine>("db.dat");
       // ... use storage ...
   }  // Destructor calls flush_all()

Out of Memory
~~~~~~~~~~~~~

**Symptoms:** ``std::bad_alloc`` or excessive memory usage

**Cause:** Cache size too large

**Solution:** Reduce cache size

.. code-block:: cpp

   // Before (512MB cache - too large!)
   StorageEngine storage("db.dat", 131072);

   // After (1MB cache)
   StorageEngine storage("db.dat", 256);

Slow Performance
~~~~~~~~~~~~~~~~

**Symptoms:** Queries take a long time

**Possible Causes:**

1. Cache too small (excessive disk I/O)
2. Not flushing efficiently (flushing too often)
3. Fragmented free list (not currently optimized)

**Solutions:**

- Increase cache size
- Batch writes before flushing
- See :doc:`../guides/performance` for more tips

See Also
--------

- :doc:`index` - B-tree index storage
- :doc:`catalog` - System catalog storage
- :doc:`../guides/architecture` - Overall architecture
- :doc:`../guides/performance` - Performance tuning
- :doc:`database` - High-level database API

**Related Classes:**

- ``Database`` - Uses StorageEngine internally
- ``Table<T>`` - Stores records using pages
- ``BTreeIndex`` - Stores index nodes using pages
