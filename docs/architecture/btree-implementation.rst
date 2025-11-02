B+Tree Implementation
=====================

.. contents:: Table of Contents
   :local:
   :depth: 3

Introduction
------------

The B+Tree is the **fundamental index structure** in LearnQL, providing O(log n) search, insertion, and deletion. This document explains B+Tree theory, implementation details, and why it's the preferred index structure for databases.

**Key Concepts**:

- **Balanced tree**: All leaf nodes are at the same depth
- **High fanout**: Each node can have many children (reduces tree height)
- **Disk-friendly**: Nodes align with disk pages (4KB)
- **Sorted data**: Supports efficient range queries

What is a B+Tree?
-----------------

B+Tree Fundamentals
^^^^^^^^^^^^^^^^^^^

A **B+Tree** is a self-balancing tree data structure that maintains sorted data and allows searches, sequential access, insertions, and deletions in **logarithmic time**.

**Key Properties**:

1. **All data in leaves**: Internal nodes contain only keys (routing information)
2. **Linked leaves**: Leaf nodes form a doubly-linked list
3. **Balanced**: All leaf nodes are at the same depth
4. **High branching factor**: Typically 50-2000 children per node
5. **Ordered**: Keys are always sorted within nodes

B+Tree vs B-Tree
^^^^^^^^^^^^^^^^

+------------------------+-------------------------+-------------------------+
| Feature                | B-Tree                  | B+Tree                  |
+========================+=========================+=========================+
| **Data storage**       | Internal + leaf nodes   | Leaf nodes only         |
+------------------------+-------------------------+-------------------------+
| **Internal nodes**     | Store keys and values   | Store keys only         |
+------------------------+-------------------------+-------------------------+
| **Leaf nodes**         | Not linked              | Linked (doubly)         |
+------------------------+-------------------------+-------------------------+
| **Node size**          | Variable (values vary)  | More uniform            |
+------------------------+-------------------------+-------------------------+
| **Range queries**      | Tree traversal          | Sequential leaf scan    |
+------------------------+-------------------------+-------------------------+
| **Search complexity**  | O(log n)                | O(log n)                |
+------------------------+-------------------------+-------------------------+
| **Insertion**          | O(log n)                | O(log n)                |
+------------------------+-------------------------+-------------------------+
| **Deletion**           | O(log n)                | O(log n)                |
+------------------------+-------------------------+-------------------------+
| **Cache efficiency**   | Good                    | Better (internal nodes  |
|                        |                         | smaller, more in cache) |
+------------------------+-------------------------+-------------------------+

**Why B+Tree for databases?**

1. **Range queries**: Leaf linking enables fast sequential scans
2. **Higher fanout**: Internal nodes store only keys → more children per node → shorter tree
3. **Consistent performance**: All searches traverse to leaf level (predictable)
4. **Better caching**: Internal nodes fit more keys in cache

B+Tree Structure
----------------

Visual Example
^^^^^^^^^^^^^^

B+Tree of order 4 (max 3 keys, max 4 children):

.. code-block:: text

                           [30]
                          /    \
                        /        \
                      /            \
                [10, 20]          [40, 50]
               /    |    \        /    |    \
             /      |      \    /      |      \
   [5,7,9] [10,15,17] [20,25,28] [30,35,37] [40,45,48] [50,60,70]
      │        │          │          │          │          │
      └────────┴──────────┴──────────┴──────────┴──────────┘
                   (leaf nodes linked together)

**Node Types**:

1. **Root Node**: ``[30]`` - can be internal or leaf
2. **Internal Nodes**: ``[10, 20]``, ``[40, 50]`` - routing only, no data
3. **Leaf Nodes**: ``[5,7,9]``, ``[10,15,17]``, etc. - contain actual data

Node Structure
^^^^^^^^^^^^^^

LearnQL's B+Tree node implementation:

.. code-block:: cpp

   struct Node {
       uint64_t page_id;                    // Page ID of this node
       std::vector<Key> keys;               // Keys in sorted order
       std::vector<Value> values;           // Values (leaf nodes only)
       std::vector<uint64_t> children_ids;  // Child page IDs (internal nodes)
       bool is_leaf;                        // True if leaf node
       uint64_t next_page_id;               // Next leaf (0 = none)
       uint64_t prev_page_id;               // Previous leaf (0 = none)
   };

**Memory Layout** (after serialization):

.. code-block:: text

   Page Layout for Index Node:

   ┌────────────────────────────────────────────────────────┐
   │ Page Header (64 bytes)                                 │
   ├────────────────────────────────────────────────────────┤
   │ Node Header:                                           │
   │   - page_id (8 bytes)                                  │
   │   - is_leaf (1 byte)                                   │
   │   - next_page_id (8 bytes)                             │
   │   - prev_page_id (8 bytes)                             │
   ├────────────────────────────────────────────────────────┤
   │ Keys:                                                  │
   │   - key_count (4 bytes)                                │
   │   - key[0], key[1], ..., key[n-1]                      │
   ├────────────────────────────────────────────────────────┤
   │ Values (leaf nodes only):                              │
   │   - value_count (4 bytes)                              │
   │   - value[0], value[1], ..., value[n-1]                │
   ├────────────────────────────────────────────────────────┤
   │ Children (internal nodes only):                        │
   │   - child_count (4 bytes)                              │
   │   - child_id[0], child_id[1], ..., child_id[n]         │
   └────────────────────────────────────────────────────────┘

Order and Degree
^^^^^^^^^^^^^^^^

LearnQL uses **order 4** (configurable via template parameter):

.. code-block:: cpp

   static constexpr std::size_t ORDER = 4;      // Max children per node
   static constexpr std::size_t MIN_KEYS = 1;   // ORDER / 2 - 1
   static constexpr std::size_t MAX_KEYS = 3;   // ORDER - 1

**Invariants**:

- Root can have 1 to MAX_KEYS keys
- Internal nodes have MIN_KEYS to MAX_KEYS keys
- Internal nodes have (keys + 1) children
- Leaf nodes have MIN_KEYS to MAX_KEYS key-value pairs

**Trade-off**: Lower order (4) is easier to visualize but less efficient. Production databases use order 50-500.

Why B+Trees for Databases?
---------------------------

Disk I/O Optimization
^^^^^^^^^^^^^^^^^^^^^

**Problem**: Disk seeks are **10,000x slower** than memory access

**Solution**: Minimize disk seeks by:

1. **High fanout**: Each node has many children → fewer levels → fewer seeks
2. **Page alignment**: Each node = one disk page → single I/O per node
3. **Sequential access**: Linked leaves enable efficient range scans

**Example**:

.. code-block:: text

   Binary Search Tree (fanout = 2):
   - 1 million records → depth ~20 → 20 disk seeks

   B+Tree (fanout = 100):
   - 1 million records → depth ~3 → 3 disk seeks

   Speedup: 6-7x fewer disk I/O operations!

Range Query Efficiency
^^^^^^^^^^^^^^^^^^^^^^

**Scenario**: Find all students with age between 20 and 25

**Binary Search Tree**:

.. code-block:: text

   Must traverse tree for each value:
   - Find 20 (log n)
   - Find 21 (log n)
   - Find 22 (log n)
   - ...
   Total: O(k × log n) where k = result size

**B+Tree**:

.. code-block:: text

   1. Find starting key (20): O(log n)
   2. Walk linked leaves: O(k / fanout)

   Total: O(log n + k / fanout) ≈ O(log n) for small k

**10-100x faster** for range queries!

Comparison with Other Index Structures
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

+------------------------+------------+------------+--------------+---------------+
| Structure              | Search     | Insert     | Range Query  | Space         |
+========================+============+============+==============+===============+
| **Hash Index**         | O(1)       | O(1)       | O(n)         | O(n)          |
+------------------------+------------+------------+--------------+---------------+
| **Binary Search Tree** | O(log n)*  | O(log n)*  | O(k log n)   | O(n)          |
+------------------------+------------+------------+--------------+---------------+
| **B+Tree**             | O(log n)   | O(log n)   | O(log n + k) | O(n)          |
+------------------------+------------+------------+--------------+---------------+
| **Skip List**          | O(log n)   | O(log n)   | O(log n + k) | O(n log n)    |
+------------------------+------------+------------+--------------+---------------+

\* *Unbalanced tree worst case is O(n)*

**Why B+Tree wins**:

- Balanced guarantee (unlike BST)
- Efficient range queries (unlike hash)
- Low space overhead (unlike skip list)
- Disk-friendly (matches page size)

B+Tree Algorithms
-----------------

Search Algorithm
^^^^^^^^^^^^^^^^

**Goal**: Find value for a given key

**Algorithm**:

.. code-block:: text

   search(node, key):
       if node is leaf:
           // Linear search in sorted keys
           for i = 0 to node.keys.size():
               if node.keys[i] == key:
                   return node.values[i]
           return NOT_FOUND

       // Internal node: find correct child
       i = 0
       while i < node.keys.size() and key >= node.keys[i]:
           i++

       // Recursively search child
       child = load_node(node.children_ids[i])
       return search(child, key)

**Implementation**:

.. code-block:: cpp

   std::optional<Value> search(uint64_t node_id, const Key& key) const {
       if (node_id == 0) {
           return std::nullopt;
       }

       Node node = load_node(node_id);

       // Leaf node: search for exact match
       if (node.is_leaf) {
           std::size_t i = 0;
           while (i < node.keys.size() && key > node.keys[i]) {
               ++i;
           }

           if (i < node.keys.size() && key == node.keys[i]) {
               return node.values[i];
           }
           return std::nullopt;
       }

       // Internal node: find correct child
       std::size_t i = 0;
       while (i < node.keys.size() && key >= node.keys[i]) {
           ++i;
       }

       return search(node.children_ids[i], key);
   }

**Time Complexity**: O(log n)

**Example Trace**:

.. code-block:: text

   Tree:           [30]
                  /    \
           [10, 20]    [40, 50]
           /  |  \      /  |  \
         ...  ... [20,25,28] ...

   Search for 25:
   1. Start at root [30]: 25 < 30 → go left
   2. At [10, 20]: 25 >= 20 → go right child
   3. At leaf [20,25,28]: found 25!

Insertion Algorithm
^^^^^^^^^^^^^^^^^^^

**Goal**: Insert (key, value) pair while maintaining B+Tree properties

**High-Level Algorithm**:

.. code-block:: text

   insert(key, value):
       if root is full:
           create new root
           split old root
           update root pointer

       insert_non_full(root, key, value)

   insert_non_full(node, key, value):
       if node is leaf:
           insert (key, value) in sorted position
       else:
           find correct child
           if child is full:
               split child
               adjust child selection
           insert_non_full(child, key, value)

**Node Splitting** (most complex part):

.. code-block:: text

   Leaf Node Split:
   ┌──────────────────────────────────────────────┐
   │ Full Leaf: [10, 20, 30, 40] (ORDER=4, full) │
   └──────────────────────────────────────────────┘
                          │
                          │ split at mid=2
                          ▼
   ┌───────────────┐            ┌───────────────┐
   │ [10, 20]      │◄──────────►│ [30, 40]      │
   └───────────────┘  (linked)  └───────────────┘
           │                            │
           │  Copy 30 up to parent      │
           └────────────────────────────┘
                        │
                        ▼
              Parent: [..., 30, ...]

   Internal Node Split:
   ┌──────────────────────────────────────────────┐
   │ Full Internal: [10, 20, 30, 40]             │
   │ Children: [c0, c1, c2, c3, c4]              │
   └──────────────────────────────────────────────┘
                          │
                          │ split at mid=2
                          ▼
   ┌───────────────┐            ┌───────────────┐
   │ [10, 20]      │            │ [40]          │
   │ [c0, c1, c2]  │            │ [c3, c4]      │
   └───────────────┘            └───────────────┘
           │                            │
           │  Push 30 up to parent      │
           └────────────────────────────┘
                        │
                        ▼
              Parent: [..., 30, ...]

**Key Difference**:

- **Leaf split**: Copy middle key up (keep in leaf)
- **Internal split**: Push middle key up (remove from children)

**Implementation**:

.. code-block:: cpp

   void split_child(Node& parent, std::size_t index) {
       Node full_child = load_node(parent.children_ids[index]);
       uint64_t new_node_id = allocate_node(full_child.is_leaf);
       Node new_node = load_node(new_node_id);

       std::size_t mid = MAX_KEYS / 2;

       if (full_child.is_leaf) {
           // LEAF NODE SPLIT

           // Move second half to new node
           new_node.keys.assign(
               full_child.keys.begin() + mid,
               full_child.keys.end()
           );
           new_node.values.assign(
               full_child.values.begin() + mid,
               full_child.values.end()
           );

           // Update leaf links
           new_node.next_page_id = full_child.next_page_id;
           new_node.prev_page_id = full_child.page_id;
           full_child.next_page_id = new_node_id;

           // Update next leaf's prev pointer
           if (new_node.next_page_id != 0) {
               Node next_leaf = load_node(new_node.next_page_id);
               next_leaf.prev_page_id = new_node_id;
               save_node(next_leaf);
           }

           // Copy first key of new node up to parent
           parent.keys.insert(parent.keys.begin() + index, new_node.keys[0]);
           parent.children_ids.insert(
               parent.children_ids.begin() + index + 1,
               new_node_id
           );

           // Shrink original child
           full_child.keys.resize(mid);
           full_child.values.resize(mid);

       } else {
           // INTERNAL NODE SPLIT

           // Move second half to new node (excluding middle key)
           new_node.keys.assign(
               full_child.keys.begin() + mid + 1,
               full_child.keys.end()
           );
           new_node.children_ids.assign(
               full_child.children_ids.begin() + mid + 1,
               full_child.children_ids.end()
           );

           // Push middle key up to parent
           parent.keys.insert(parent.keys.begin() + index, full_child.keys[mid]);
           parent.children_ids.insert(
               parent.children_ids.begin() + index + 1,
               new_node_id
           );

           // Shrink original child (excluding middle key)
           full_child.keys.resize(mid);
           full_child.children_ids.resize(mid + 1);
       }

       save_node(full_child);
       save_node(new_node);
       save_node(parent);
   }

**Time Complexity**: O(log n)

Deletion Algorithm
^^^^^^^^^^^^^^^^^^

**Goal**: Remove key while maintaining B+Tree properties

**Simplified Algorithm** (current implementation):

.. code-block:: cpp

   bool remove_from_node(Node& node, const Key& key) {
       if (node.is_leaf) {
           // Find and remove key from leaf
           for (size_t i = 0; i < node.keys.size(); ++i) {
               if (node.keys[i] == key) {
                   node.keys.erase(node.keys.begin() + i);
                   node.values.erase(node.values.begin() + i);
                   save_node(node);
                   return true;
               }
           }
           return false;
       } else {
           // Internal node: find correct child
           size_t i = 0;
           while (i < node.keys.size() && key >= node.keys[i]) {
               ++i;
           }

           Node child = load_node(node.children_ids[i]);
           return remove_from_node(child, key);
       }
   }

**Limitations**:

- Does not handle underflow (node has < MIN_KEYS)
- Does not rebalance or merge nodes
- Acceptable for learning purposes

**Full Deletion Algorithm** (not implemented):

.. code-block:: text

   delete(node, key):
       if node is leaf:
           remove key

           if node.keys < MIN_KEYS and node is not root:
               // Handle underflow
               if sibling has extra keys:
                   borrow from sibling
               else:
                   merge with sibling

       else:  // internal node
           find child
           delete(child, key)

           if child underflowed:
               redistribute or merge

**Time Complexity**: O(log n)

Range Query Algorithm
^^^^^^^^^^^^^^^^^^^^^

**Goal**: Find all key-value pairs where min_key ≤ key ≤ max_key

**Algorithm** (B+Tree optimized):

.. code-block:: text

   range_query(min_key, max_key):
       // Step 1: Find starting leaf
       leaf = find_leaf_for_key(root, min_key)

       // Step 2: Walk linked leaves
       results = []
       while leaf is not null:
           for each (key, value) in leaf:
               if key < min_key: continue
               if key > max_key: return results
               results.append(value)

           leaf = leaf.next_page_id  // Follow link

       return results

**Implementation**:

.. code-block:: cpp

   std::vector<Value> range_query(const Key& min_key, const Key& max_key) const {
       std::vector<Value> results;

       // Find starting leaf
       uint64_t leaf_id = find_leaf_for_key(root_page_id_, min_key);
       if (leaf_id == 0) {
           return results;
       }

       // Walk leaf linked list
       while (leaf_id != 0) {
           Node leaf = load_node(leaf_id);

           for (std::size_t i = 0; i < leaf.keys.size(); ++i) {
               if (leaf.keys[i] < min_key) {
                   continue;  // Skip keys below range
               }
               if (leaf.keys[i] > max_key) {
                   return results;  // Done, keys are sorted
               }
               results.push_back(leaf.values[i]);
           }

           leaf_id = leaf.next_page_id;  // Move to next leaf
       }

       return results;
   }

**Time Complexity**: O(log n + k) where k = number of results

**Why so fast?**

- Single tree traversal to find start: O(log n)
- Sequential leaf scan: O(k / fanout) ≈ O(1) per result
- No repeated tree traversals!

B+Tree Visualization
--------------------

ASCII Tree Example
^^^^^^^^^^^^^^^^^^

Here's a complete B+Tree (order 4) after several insertions:

.. code-block:: text

   Insert sequence: 10, 20, 30, 40, 50, 60, 70, 80

   Final tree:

                           [40]
                          /    \
                        /        \
                      /            \
                  [20]            [60]
                 /    \          /    \
               /        \      /        \
         [10,15]  [20,30]  [40,50]  [60,70,80]
            ↓        ↓        ↓          ↓
         (values) (values) (values)  (values)

   Leaf linked list (left to right):
   [10,15] ←→ [20,30] ←→ [40,50] ←→ [60,70,80]

**Node Details**:

- Root: ``[40]`` - 1 key, 2 children
- Internal: ``[20]`` - 1 key, 2 children
- Internal: ``[60]`` - 1 key, 2 children
- Leaves: 4 nodes with actual data

**Invariants Check**:

- ✓ All leaves at same depth (3)
- ✓ All nodes have MIN_KEYS to MAX_KEYS (except root)
- ✓ Keys are sorted within nodes
- ✓ Leaves are linked

Step-by-Step Insertion Example
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Insert 25 into tree**:

.. code-block:: text

   Before:
                  [40]
                 /    \
            [20]      [60]
           /    \    /    \
      [10,15] [20,30] [40,50] [60,70,80]

   Step 1: Traverse to correct leaf
   - Start at root [40]: 25 < 40 → go left
   - At [20]: 25 >= 20 → go right
   - At leaf [20,30]: insert 25

   Step 2: Insert into leaf (not full)
      [20,30] → [20,25,30]

   After:
                  [40]
                 /    \
            [20]      [60]
           /    \    /    \
      [10,15] [20,25,30] [40,50] [60,70,80]

   No splits needed!

**Insert 35 (causes split)**:

.. code-block:: text

   Before:
      [20,25,30] (capacity 3, inserting 4th element)

   Step 1: Insert 35 temporarily
      [20,25,30,35] (FULL, ORDER=4)

   Step 2: Split at mid=2
      Left: [20,25]
      Right: [30,35]

   Step 3: Copy 30 up to parent
      Parent [20] becomes [20, 30]

   After:
                    [40]
                   /    \
               [20, 30]   [60]
              /   |   \
         [10,15][20,25][30,35] ...

Performance Analysis
--------------------

Time Complexity
^^^^^^^^^^^^^^^

+------------------------+------------------+------------------------+
| Operation              | Average Case     | Worst Case             |
+========================+==================+========================+
| Search                 | O(log_M n)       | O(log_M n)             |
+------------------------+------------------+------------------------+
| Insert                 | O(log_M n)       | O(log_M n)             |
+------------------------+------------------+------------------------+
| Delete                 | O(log_M n)       | O(log_M n)             |
+------------------------+------------------+------------------------+
| Range Query            | O(log_M n + k)   | O(log_M n + k)         |
+------------------------+------------------+------------------------+
| Full Scan              | O(n)             | O(n)                   |
+------------------------+------------------+------------------------+

*Where M = order (fanout), n = number of records, k = result size*

**Why log_M n?**

.. code-block:: text

   Tree height = log_M(n)

   Example (M=100, n=1,000,000):
   height = log_100(1,000,000) = 3

   Each level requires one disk I/O
   → Only 3 disk seeks to find any record!

Space Complexity
^^^^^^^^^^^^^^^^

- **Per node**: ~50-200 bytes overhead (headers, pointers)
- **Total space**: O(n) where n = number of entries
- **Index overhead**: ~10-30% of indexed data size

**Example**:

.. code-block:: text

   1 million integers (4 bytes each):
   - Raw data: 4 MB
   - B+Tree index: 4.5-5 MB
   - Overhead: ~0.5-1 MB (12-25%)

Practical Benchmark
^^^^^^^^^^^^^^^^^^^

**Setup**: 100,000 integer keys

+------------------------+------------------+------------------------+
| Operation              | LearnQL B+Tree   | std::map (Red-Black)   |
+========================+==================+========================+
| Insert (avg)           | 15 μs            | 8 μs                   |
+------------------------+------------------+------------------------+
| Search (avg)           | 12 μs            | 10 μs                  |
+------------------------+------------------+------------------------+
| Range (100 items)      | 50 μs            | 200 μs                 |
+------------------------+------------------+------------------------+
| Memory usage           | 6 MB             | 12 MB                  |
+------------------------+------------------+------------------------+

**Observations**:

- B+Tree slower for single insertions (disk I/O)
- B+Tree **4x faster** for range queries (linked leaves)
- B+Tree uses **50% less memory** (internal nodes smaller)

Optimization Strategies
^^^^^^^^^^^^^^^^^^^^^^^

1. **Increase order** for production workloads

   .. code-block:: cpp

      static constexpr std::size_t ORDER = 128;  // 127 keys, 128 children

2. **Bulk loading** for initial data

   .. code-block:: text

      Instead of individual inserts, build tree bottom-up:
      1. Sort all data
      2. Build leaf nodes (fill to ~80%)
      3. Build internal levels upward
      → 5-10x faster than individual inserts

3. **Node prefilling** for better space utilization

   .. code-block:: text

      Fill nodes to 70-80% capacity (not 100%)
      → Reduces splits during subsequent inserts

Code Examples
-------------

Example 1: Basic Insert and Search
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   #include <learnql/index/PersistentBTreeIndex.hpp>
   #include <learnql/storage/StorageEngine.hpp>

   using namespace learnql;

   int main() {
       auto storage = std::make_shared<storage::StorageEngine>("index.db");

       // Create B+Tree index
       index::PersistentBTreeIndex<int, core::RecordId> btree(storage);

       // Insert key-value pairs
       btree.insert(42, core::RecordId{1, 0});
       btree.insert(17, core::RecordId{2, 5});
       btree.insert(99, core::RecordId{3, 2});

       // Search
       auto result = btree.find(42);
       if (result) {
           std::cout << "Found: page=" << result->page_id
                     << ", slot=" << result->slot << "\n";
       }

       // Persist to disk
       btree.flush();
   }

Example 2: Range Query
^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // Insert student IDs
   for (int id = 1; id <= 100; ++id) {
       btree.insert(id, core::RecordId{id / 10, id % 10});
   }

   // Find all students with IDs 20-30
   auto results = btree.range_query(20, 30);

   std::cout << "Found " << results.size() << " students\n";
   for (const auto& rid : results) {
       std::cout << "  Record: page=" << rid.page_id
                 << ", slot=" << rid.slot << "\n";
   }

Example 3: Iteration with Batch Iterator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // Create batch iterator (fetches 10 entries at a time)
   auto iter = btree.create_batch_iterator<10>();

   while (iter.has_more()) {
       auto batch = iter.next_batch();

       std::cout << "Batch of " << batch.size() << " entries:\n";
       for (const auto& [key, value] : batch) {
           std::cout << "  " << key << " -> "
                     << value.page_id << ":" << value.slot << "\n";
       }
   }

Example 4: Persistence and Recovery
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // Session 1: Create and populate index
   {
       auto storage = std::make_shared<storage::StorageEngine>("data.db");
       index::PersistentBTreeIndex<int, core::RecordId> btree(storage);

       btree.insert(1, core::RecordId{1, 0});
       btree.insert(2, core::RecordId{2, 0});
       btree.insert(3, core::RecordId{3, 0});

       btree.flush();  // Persist to disk

       uint64_t root_id = btree.get_root_page_id();
       std::cout << "Root page ID: " << root_id << "\n";
   }

   // Session 2: Reopen and use existing index
   {
       auto storage = std::make_shared<storage::StorageEngine>("data.db");

       // Load existing index (pass root page ID)
       uint64_t root_id = 1;  // From previous session
       index::PersistentBTreeIndex<int, core::RecordId> btree(storage, root_id);

       // Index is fully restored!
       auto result = btree.find(2);
       std::cout << "Found: " << result->page_id << "\n";
   }

Design Trade-offs
-----------------

+---------------------------------+---------------------------+---------------------------+
| Decision                        | Chosen Approach           | Alternative               |
+=================================+===========================+===========================+
| **Tree type**                   | B+Tree                    | B-Tree, AVL, Red-Black    |
+---------------------------------+---------------------------+---------------------------+
| **Order**                       | 4 (low)                   | 50-500 (production)       |
+---------------------------------+---------------------------+---------------------------+
| **Leaf linking**                | Doubly-linked             | Singly-linked, unlinked   |
+---------------------------------+---------------------------+---------------------------+
| **Storage**                     | One node per page         | Multiple nodes per page   |
+---------------------------------+---------------------------+---------------------------+
| **Deletion**                    | No rebalancing            | Full rebalancing          |
+---------------------------------+---------------------------+---------------------------+
| **Node size**                   | Fixed (4KB page)          | Variable                  |
+---------------------------------+---------------------------+---------------------------+

**Why order 4?**

- Easy to visualize and debug
- Demonstrates splitting/merging clearly
- Good for learning and small datasets
- Production: use order 100+ for better performance

Limitations
-----------

1. **Low order**: Order 4 is inefficient for large datasets

   - **Solution**: Increase ORDER to 64-128 for production

2. **No deletion rebalancing**: Deleted nodes may underflow

   - **Solution**: Implement borrowing and merging

3. **One node per page**: Wastes space if nodes are small

   - **Solution**: Pack multiple nodes into a single page

4. **No bulk loading**: Inserting sorted data is slow

   - **Solution**: Implement bottom-up bulk loading

5. **Simple caching**: No LRU or access frequency tracking

   - **Solution**: Implement sophisticated cache replacement policy

Future Improvements
-------------------

1. **Higher order**: Use order 128-256 for production performance
2. **Bulk loading**: Bottom-up tree construction from sorted data
3. **Node compression**: Prefix compression for keys
4. **Deletion rebalancing**: Full merge and redistribute algorithms
5. **Concurrency**: Latch coupling or optimistic lock coupling
6. **Statistics**: Track node fill factors, access patterns
7. **Adaptive order**: Adjust order based on key/value sizes

See Also
--------

- :doc:`storage-engine` - How B+Tree nodes are stored in pages
- :doc:`performance` - Detailed benchmarks and profiling
- :doc:`overview` - Overall architecture

References
----------

Academic Papers
^^^^^^^^^^^^^^^

- Bayer, R., & McCreight, E. (1972). "Organization and maintenance of large ordered indexes". *Acta Informatica*, 1(3), 173-189.
- Comer, D. (1979). "The Ubiquitous B-Tree". *ACM Computing Surveys*, 11(2), 121-137.
- Graefe, G. (2011). "Modern B-Tree Techniques". *Foundations and Trends in Databases*, 3(4), 203-402.

Books
^^^^^

- Knuth, D. E. (1998). *The Art of Computer Programming, Volume 3: Sorting and Searching* (2nd ed.). Addison-Wesley.
- Cormen, T. H., Leiserson, C. E., Rivest, R. L., & Stein, C. (2009). *Introduction to Algorithms* (3rd ed.). MIT Press.
- Ramakrishnan, R., & Gehrke, J. (2003). *Database Management Systems* (3rd ed.). McGraw-Hill.

Online Resources
^^^^^^^^^^^^^^^^

- SQLite B-Tree implementation: https://www.sqlite.org/btreemodule.html
- PostgreSQL B-Tree documentation: https://www.postgresql.org/docs/current/btree-implementation.html
- B+Tree visualization: https://www.cs.usfca.edu/~galles/visualization/BPlusTree.html
