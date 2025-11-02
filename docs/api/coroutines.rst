Coroutines Module
=================

.. contents:: Table of Contents
   :local:
   :depth: 2

Overview
--------

The Coroutines module provides C++20 coroutine-based generators for lazy sequence generation. Generators enable memory-efficient processing of large datasets by yielding values one at a time instead of materializing entire result sets in memory.

**Key Features:**

- C++20 coroutine support (``co_yield``, ``co_return``)
- Lazy value generation with minimal memory footprint
- Iterator interface for range-based for loops
- Exception propagation from coroutines
- Zero-copy value yielding

**Module Components:**

- ``Generator<T>`` - Coroutine-based lazy sequence generator
- Promise type for coroutine state management
- Iterator support for standard algorithms

.. note::
   This module requires a C++20-compliant compiler with coroutine support (GCC 10+, Clang 12+, MSVC 2019+).

Quick Start
-----------

Basic Generator
~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <learnql/coroutines/Generator.hpp>

   // Define a generator function
   Generator<int> count_to(int n) {
       for (int i = 0; i < n; ++i) {
           co_yield i;
       }
   }

   // Use the generator
   for (int value : count_to(5)) {
       std::cout << value << " ";  // Output: 0 1 2 3 4
   }

Infinite Generator
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   Generator<int> fibonacci() {
       int a = 0, b = 1;
       while (true) {
           co_yield a;
           int tmp = a + b;
           a = b;
           b = tmp;
       }
   }

   // Take first 10 Fibonacci numbers
   auto fib = fibonacci();
   for (int i = 0; i < 10; ++i) {
       std::cout << fib.next().value() << " ";
   }
   // Output: 0 1 1 2 3 5 8 13 21 34

Generator with Database
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Lazy-load students in batches
   Generator<Student> load_students(Table<Student>& table) {
       for (auto& student : table.all()) {
           co_yield student;
       }
   }

   // Process one at a time (memory efficient)
   for (auto student : load_students(students)) {
       process(student);  // Only one student in memory at a time
   }

Class Reference
---------------

Generator Class
~~~~~~~~~~~~~~~

.. doxygenclass:: learnql::coroutines::Generator
   :members:

A coroutine-based generator that yields values lazily.

**Template Parameters:**

- ``T`` - Type of values generated

**Key Characteristics:**

- Lazy evaluation - values generated on demand
- Resumable - coroutine state preserved between calls
- Memory efficient - only current value in memory
- Move-only - cannot be copied (owns coroutine state)

Constructor
^^^^^^^^^^^

.. code-block:: cpp

   explicit Generator(handle_type h);

Typically constructed by coroutine infrastructure (not directly by users).

**Example:**

.. code-block:: cpp

   // Compiler generates constructor call
   Generator<int> gen = my_generator_function();

Move Constructor/Assignment
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   Generator(Generator&& other) noexcept;
   Generator& operator=(Generator&& other) noexcept;

Generators are move-only (cannot be copied).

**Example:**

.. code-block:: cpp

   Generator<int> gen1 = count_to(10);
   Generator<int> gen2 = std::move(gen1);  // OK - move
   // Generator<int> gen3 = gen2;          // ERROR - no copy

Destructor
^^^^^^^^^^

.. code-block:: cpp

   ~Generator();

Automatically destroys the coroutine when the generator goes out of scope.

Core Methods
^^^^^^^^^^^^

``next()`` - Get Next Value
""""""""""""""""""""""""""""

.. code-block:: cpp

   std::optional<T> next();

Resumes the coroutine and returns the next yielded value.

**Returns:** ``std::optional<T>`` containing the next value, or ``std::nullopt`` if done

**Example:**

.. code-block:: cpp

   Generator<int> gen = count_to(3);

   auto v1 = gen.next();  // Returns std::optional<int>{0}
   auto v2 = gen.next();  // Returns std::optional<int>{1}
   auto v3 = gen.next();  // Returns std::optional<int>{2}
   auto v4 = gen.next();  // Returns std::nullopt (done)

``has_next()`` - Check if More Values
""""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   [[nodiscard]] bool has_next() const;

Checks if the generator has more values to yield.

**Returns:** ``true`` if more values available

**Example:**

.. code-block:: cpp

   Generator<int> gen = count_to(5);

   while (gen.has_next()) {
       auto value = gen.next();
       std::cout << value.value() << "\n";
   }

Iterator Support
^^^^^^^^^^^^^^^^

Generators support iteration via iterators:

.. code-block:: cpp

   Iterator begin();
   Iterator end();

**Example:**

.. code-block:: cpp

   Generator<int> gen = count_to(5);

   // Range-based for loop
   for (int value : gen) {
       std::cout << value << "\n";
   }

   // STL algorithms
   Generator<int> gen2 = count_to(10);
   auto it = std::find(gen2.begin(), gen2.end(), 5);

Promise Type
~~~~~~~~~~~~

The ``promise_type`` manages coroutine state (typically not used directly):

.. code-block:: cpp

   struct promise_type {
       std::optional<T> current_value;
       std::exception_ptr exception;

       Generator get_return_object();
       std::suspend_always initial_suspend() noexcept;
       std::suspend_always final_suspend() noexcept;
       std::suspend_always yield_value(T value) noexcept;
       void return_void() noexcept;
       void unhandled_exception();
   };

Exception Handling
^^^^^^^^^^^^^^^^^^

Exceptions thrown in the coroutine are captured and rethrown when calling ``next()``:

.. code-block:: cpp

   Generator<int> throwing_gen() {
       co_yield 1;
       throw std::runtime_error("Error!");
       co_yield 2;  // Never reached
   }

   try {
       auto gen = throwing_gen();
       auto v1 = gen.next();  // OK - returns 1
       auto v2 = gen.next();  // Throws std::runtime_error
   } catch (const std::exception& e) {
       std::cout << "Caught: " << e.what() << "\n";
   }

Usage Examples
--------------

Fibonacci Sequence
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   Generator<uint64_t> fibonacci(std::size_t limit) {
       uint64_t a = 0, b = 1;
       for (std::size_t i = 0; i < limit; ++i) {
           co_yield a;
           uint64_t tmp = a + b;
           a = b;
           b = tmp;
       }
   }

   // Print first 15 Fibonacci numbers
   for (auto num : fibonacci(15)) {
       std::cout << num << " ";
   }
   // Output: 0 1 1 2 3 5 8 13 21 34 55 89 144 233 377

Prime Number Generator
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   bool is_prime(int n) {
       if (n < 2) return false;
       for (int i = 2; i * i <= n; ++i) {
           if (n % i == 0) return false;
       }
       return true;
   }

   Generator<int> primes(int limit) {
       for (int i = 2; i < limit; ++i) {
           if (is_prime(i)) {
               co_yield i;
           }
       }
   }

   // Get first 10 primes
   auto prime_gen = primes(100);
   for (int i = 0; i < 10; ++i) {
       if (auto p = prime_gen.next()) {
           std::cout << *p << " ";
       }
   }
   // Output: 2 3 5 7 11 13 17 19 23 29

Range Generator
~~~~~~~~~~~~~~~

.. code-block:: cpp

   template<typename T>
   Generator<T> range(T start, T end, T step = 1) {
       for (T i = start; i < end; i += step) {
           co_yield i;
       }
   }

   // Use like Python's range()
   for (int i : range(0, 10, 2)) {
       std::cout << i << " ";  // Output: 0 2 4 6 8
   }

   for (double d : range(0.0, 1.0, 0.1)) {
       std::cout << d << " ";  // Output: 0.0 0.1 0.2 ... 0.9
   }

Lazy Filter
~~~~~~~~~~~

.. code-block:: cpp

   template<typename T, typename Pred>
   Generator<T> filter(Generator<T>& source, Pred predicate) {
       for (auto value : source) {
           if (predicate(value)) {
               co_yield value;
           }
       }
   }

   // Generate even numbers
   auto numbers = range(0, 20);
   auto evens = filter(numbers, [](int n) { return n % 2 == 0; });

   for (int even : evens) {
       std::cout << even << " ";  // Output: 0 2 4 6 8 10 12 14 16 18
   }

Lazy Map
~~~~~~~~

.. code-block:: cpp

   template<typename T, typename F>
   Generator<std::invoke_result_t<F, T>> map(Generator<T>& source, F func) {
       for (auto value : source) {
           co_yield func(value);
       }
   }

   // Square numbers
   auto numbers = range(1, 6);
   auto squared = map(numbers, [](int n) { return n * n; });

   for (int sq : squared) {
       std::cout << sq << " ";  // Output: 1 4 9 16 25
   }

Batch Reader
~~~~~~~~~~~~

.. code-block:: cpp

   template<typename T>
   Generator<std::vector<T>> batch_reader(
       Table<T>& table,
       std::size_t batch_size
   ) {
       std::vector<T> batch;
       batch.reserve(batch_size);

       for (auto& record : table.all()) {
           batch.push_back(record);

           if (batch.size() >= batch_size) {
               co_yield batch;
               batch.clear();
           }
       }

       // Yield remaining records
       if (!batch.empty()) {
           co_yield batch;
       }
   }

   // Process in batches of 100
   for (auto batch : batch_reader(students, 100)) {
       process_batch(batch);  // Process 100 students at a time
   }

Tree Traversal
~~~~~~~~~~~~~~

.. code-block:: cpp

   struct TreeNode {
       int value;
       std::vector<std::unique_ptr<TreeNode>> children;
   };

   Generator<int> traverse_depth_first(const TreeNode& node) {
       co_yield node.value;

       for (const auto& child : node.children) {
           for (int value : traverse_depth_first(*child)) {
               co_yield value;
           }
       }
   }

   // Usage
   TreeNode root{1, {}};
   root.children.push_back(std::make_unique<TreeNode>(2));
   root.children.push_back(std::make_unique<TreeNode>(3));

   for (int value : traverse_depth_first(root)) {
       std::cout << value << " ";  // Output: 1 2 3
   }

Advanced Patterns
-----------------

Generator Composition
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Chain multiple generators
   Generator<int> multiples_of(int n, int limit) {
       for (int i = n; i < limit; i += n) {
           co_yield i;
       }
   }

   Generator<int> composite_generator() {
       // Yield multiples of 2
       for (int val : multiples_of(2, 10)) {
           co_yield val;
       }

       // Then yield multiples of 3
       for (int val : multiples_of(3, 10)) {
           co_yield val;
       }
   }

   for (int n : composite_generator()) {
       std::cout << n << " ";  // Output: 2 4 6 8 3 6 9
   }

Stateful Generator
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   class Counter {
   public:
       Generator<int> count() {
           while (true) {
               co_yield count_++;
           }
       }

   private:
       int count_ = 0;
   };

   Counter counter;
   auto gen = counter.count();

   std::cout << gen.next().value() << "\n";  // 0
   std::cout << gen.next().value() << "\n";  // 1
   std::cout << gen.next().value() << "\n";  // 2

Conditional Generation
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   Generator<int> conditional_gen(bool include_negatives) {
       for (int i = 0; i < 10; ++i) {
           co_yield i;
       }

       if (include_negatives) {
           for (int i = -1; i > -10; --i) {
               co_yield i;
           }
       }
   }

   // With negatives
   for (int n : conditional_gen(true)) {
       std::cout << n << " ";  // 0 1 2... 9 -1 -2... -9
   }

Performance Considerations
--------------------------

Memory Usage
~~~~~~~~~~~~

Generators use minimal memory compared to eager evaluation:

.. code-block:: cpp

   // EAGER - loads all 1 million records into memory
   std::vector<int> all_numbers;
   for (int i = 0; i < 1'000'000; ++i) {
       all_numbers.push_back(i);
   }
   // Memory: ~4 MB (1M integers × 4 bytes)

   // LAZY - only current value in memory
   Generator<int> lazy_numbers() {
       for (int i = 0; i < 1'000'000; ++i) {
           co_yield i;
       }
   }
   // Memory: ~4 bytes (single integer)

Coroutine Overhead
~~~~~~~~~~~~~~~~~~

Coroutines have minimal overhead:

.. list-table::
   :header-rows: 1
   :widths: 40 30 30

   * - Aspect
     - Coroutine
     - Function
   * - Initial Allocation
     - ~32-64 bytes
     - Stack frame
   * - Suspend/Resume
     - ~10-20 CPU cycles
     - Function call
   * - State Management
     - Automatic
     - Manual (static vars)

**Verdict:** Coroutine overhead is negligible for most use cases.

When to Use Generators
~~~~~~~~~~~~~~~~~~~~~~

✅ **Large Datasets**

.. code-block:: cpp

   // Perfect for generators
   for (auto record : load_million_records()) {
       process(record);  // One at a time
   }

✅ **Infinite Sequences**

.. code-block:: cpp

   // Can't materialize infinite sequence
   for (int fib : fibonacci()) {
       if (fib > 1000) break;
       std::cout << fib << "\n";
   }

✅ **Computed Sequences**

.. code-block:: cpp

   // Generate on-the-fly (no storage needed)
   for (int prime : primes(1000)) {
       process(prime);
   }

❌ **Small Datasets**

.. code-block:: cpp

   // Overkill for 10 elements
   std::vector<int> small = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

❌ **Multiple Passes**

.. code-block:: cpp

   // Generators can only be iterated once
   Generator<int> gen = range(0, 10);
   for (int n : gen) { /* ... */ }  // OK
   for (int n : gen) { /* ... */ }  // ERROR - already consumed

Best Practices
--------------

1. **Use Generators for Large/Infinite Sequences**

   .. code-block:: cpp

      // GOOD - lazy, memory efficient
      Generator<Data> load_large_dataset() {
           for (auto& record : large_table) {
               co_yield record;
           }
       }

2. **Avoid Capturing by Reference**

   .. code-block:: cpp

      // DANGEROUS - dangling reference
      Generator<int> bad_gen(const std::vector<int>& vec) {
           for (int val : vec) {
               co_yield val;
           }
           // vec might be destroyed before generator finishes!
       }

      // SAFER - copy data or ensure lifetime
      Generator<int> good_gen(std::vector<int> vec) {  // Copy
           for (int val : vec) {
               co_yield val;
           }
       }

3. **Use ``std::optional`` for Nullable Values**

   .. code-block:: cpp

      Generator<std::optional<int>> find_values(const std::vector<int>& data) {
           for (int val : data) {
               if (is_interesting(val)) {
                   co_yield std::make_optional(val);
               } else {
                   co_yield std::nullopt;
               }
           }
       }

4. **Handle Exceptions**

   .. code-block:: cpp

      Generator<int> safe_gen() {
           try {
               for (int i = 0; i < 10; ++i) {
                   if (i == 5) {
                       throw std::runtime_error("Error at 5");
                   }
                   co_yield i;
               }
           } catch (...) {
               // Log or handle
               throw;  // Re-throw to caller
           }
       }

5. **Don't Return Generators by Value from Functions**

   .. code-block:: cpp

      // AVOID - unnecessary moves
      Generator<int> make_generator() {
           return count_to(10);  // OK but might move
       }

      // PREFER - use directly
      for (int n : count_to(10)) {
           // ...
       }

Limitations
-----------

Current Limitations
~~~~~~~~~~~~~~~~~~~

1. **Single-Pass Iteration**

   Generators can only be iterated once.

   .. code-block:: cpp

      Generator<int> gen = count_to(5);
      for (int n : gen) { /* ... */ }  // OK
      for (int n : gen) { /* ... */ }  // ERROR - consumed

2. **No Random Access**

   Can't jump to arbitrary positions.

   .. code-block:: cpp

      Generator<int> gen = count_to(100);
      // auto val = gen[50];  // ERROR - no operator[]

3. **No Bidirectional Iteration**

   Can't iterate backwards.

   .. code-block:: cpp

      Generator<int> gen = count_to(10);
      // for (auto it = gen.rbegin(); ...)  // ERROR - no reverse iterators

Troubleshooting
---------------

Coroutine Not Resuming
~~~~~~~~~~~~~~~~~~~~~~

**Problem:** Generator appears stuck

**Cause:** Never called ``next()`` or didn't iterate

**Solution:** Ensure iteration or explicit ``next()`` calls

.. code-block:: cpp

   Generator<int> gen = count_to(5);

   // WRONG - generator created but never used
   // (values never yielded)

   // RIGHT - iterate
   for (int n : gen) {
       std::cout << n << "\n";
   }

Dangling References
~~~~~~~~~~~~~~~~~~~

**Problem:** Crash or undefined behavior

**Cause:** Captured references to destroyed objects

**Solution:** Copy data or ensure lifetime

.. code-block:: cpp

   // WRONG
   Generator<int> bad() {
       std::vector<int> local = {1, 2, 3};
       for (int val : local) {
           co_yield val;
       }
       // local destroyed here, but generator might resume later!
   }

   // RIGHT
   Generator<int> good() {
       for (int val : {1, 2, 3}) {  // Temporary, copied into coroutine frame
           co_yield val;
       }
   }

Compiler Errors
~~~~~~~~~~~~~~~

**Problem:** "coroutines not supported" error

**Cause:** Compiler too old or coroutines not enabled

**Solution:** Use C++20 and enable coroutines

.. code-block:: bash

   # GCC
   g++ -std=c++20 -fcoroutines ...

   # Clang
   clang++ -std=c++20 -stdlib=libc++ ...

   # MSVC
   cl /std:c++20 ...

See Also
--------

- :doc:`ranges` - Ranges module (complementary to generators)
- :doc:`query` - Query module
- :doc:`../tutorials/coroutines` - Coroutines tutorial
- :doc:`../guides/performance` - Performance optimization

**Related Headers:**

- ``learnql/coroutines/Generator.hpp``
- ``learnql/coroutines/AsyncQuery.hpp``

**External Resources:**

- `C++20 Coroutines <https://en.cppreference.com/w/cpp/language/coroutines>`_
- `Coroutine TS <https://en.cppreference.com/w/cpp/experimental/coroutine>`_
