C++20 Glossary
==============

This comprehensive glossary explains C++20 terms and features used in LearnQL. Each entry includes a beginner-friendly definition, why it's used in LearnQL, and a simple example.

.. note::
   This glossary is designed for C++ beginners learning modern C++20. Terms are explained with practical examples from LearnQL's codebase.

.. contents:: Quick Navigation
   :local:
   :depth: 2

Core C++20 Features
-------------------

auto
~~~~

**Definition**: The ``auto`` keyword tells the compiler to automatically deduce the type of a variable from its initializer.

**Why LearnQL Uses It**: Reduces verbosity and makes code easier to read, especially with complex template types.

**Example**:

.. code-block:: cpp

   // Without auto
   std::vector<Student>::iterator it = students.begin();

   // With auto (type deduced automatically)
   auto it = students.begin();

   // In LearnQL queries
   auto results = students.query()
       .where(Student::age > 18)
       .collect();  // Type: std::vector<Student>

**See Also**: :doc:`../getting-started/core-concepts`, :ref:`decltype`

Concepts
~~~~~~~~

**Definition**: Concepts are named sets of requirements for template parameters. They let you specify what operations a type must support to be used with a template.

**Why LearnQL Uses It**: Provides clear, compile-time error messages and ensures only valid types are used with database operations.

**Example**:

.. code-block:: cpp

   // Define a concept
   template<typename T>
   concept Serializable = requires(T obj) {
       { obj.serialize() } -> std::convertible_to<std::vector<std::byte>>;
   };

   // Use the concept
   template<Serializable T>
   void saveToDatabase(const T& record) {
       auto data = record.serialize();
       // Save data...
   }

   // Compile error if T doesn't have serialize()!

**Why This Matters**: Without concepts, template errors can be cryptic. Concepts give clear error messages like "Student doesn't satisfy Serializable concept."

**See Also**: :doc:`../api/reflection`, :ref:`Constraints`, :ref:`Requires clause`

Constraints
~~~~~~~~~~~

**Definition**: Constraints are boolean expressions that restrict template parameters. They work with concepts to enforce requirements.

**Why LearnQL Uses It**: Ensures type safety at compile-time for database operations.

**Example**:

.. code-block:: cpp

   // Constraint using concept
   template<typename T>
       requires HasPrimaryKey<T>
   class Table {
       // Only types with primary keys can be stored
   };

   // Multiple constraints
   template<typename T>
       requires Serializable<T> && Reflectable<T>
   void insertRecord(const T& record);

**See Also**: :ref:`Concepts`, :ref:`Requires clause`

consteval
~~~~~~~~~

**Definition**: A function modifier that forces evaluation at compile-time. Unlike ``constexpr``, it cannot be evaluated at runtime.

**Why LearnQL Uses It**: Performs compile-time validation and metadata generation with zero runtime cost.

**Example**:

.. code-block:: cpp

   consteval int factorial(int n) {
       return (n <= 1) ? 1 : n * factorial(n - 1);
   }

   constexpr int x = factorial(5);  // OK: Evaluated at compile-time
   // int y = factorial(user_input);  // ERROR: Must be compile-time constant

   // In LearnQL
   consteval size_t countFields() {
       // Calculate number of fields at compile-time
       return /* field count */;
   }

**See Also**: :ref:`constexpr`, :ref:`constinit`

constexpr
~~~~~~~~~

**Definition**: Specifies that a variable or function can be evaluated at compile-time. Unlike ``consteval``, it can also run at runtime.

**Why LearnQL Uses It**: Enables compile-time computation for performance and zero-cost reflection.

**Example**:

.. code-block:: cpp

   constexpr int square(int x) {
       return x * x;
   }

   constexpr int compile_time = square(10);  // Computed at compile-time
   int runtime = square(user_input);         // Computed at runtime

   // In LearnQL
   constexpr bool isPrimaryKey() const {
       return flags_ & FieldFlags::PRIMARY_KEY;
   }

**See Also**: :ref:`consteval`, :ref:`constinit`, :doc:`../architecture/reflection-system`

constinit
~~~~~~~~~

**Definition**: Ensures a variable is initialized at compile-time with static storage duration. Unlike ``constexpr``, the variable is not constant.

**Why LearnQL Uses It**: Guarantees zero-cost initialization of global/static variables.

**Example**:

.. code-block:: cpp

   constinit int initialized_value = 42;  // Initialized at compile-time

   void modify() {
       initialized_value = 100;  // OK: Can be modified
   }

   // Compare with constexpr
   constexpr int constant_value = 42;
   // constant_value = 100;  // ERROR: Cannot modify

**See Also**: :ref:`constexpr`, :ref:`consteval`

Coroutines
~~~~~~~~~~

**Definition**: Functions that can suspend execution and resume later. They use ``co_await``, ``co_yield``, or ``co_return`` keywords.

**Why LearnQL Uses It**: Enables lazy evaluation and memory-efficient streaming of large result sets.

**Example**:

.. code-block:: cpp

   // Generator coroutine
   Generator<int> countTo(int max) {
       for (int i = 0; i < max; ++i) {
           co_yield i;  // Suspend and return value
       }
   }

   // Usage
   for (int value : countTo(5)) {
       std::cout << value << '\n';  // Prints 0, 1, 2, 3, 4
   }

   // In LearnQL
   Generator<Student> streamStudents(Table<Student>& table) {
       for (const auto& student : table.scan()) {
           co_yield student;  // Stream one at a time
       }
   }

**See Also**: :ref:`co_yield`, :ref:`co_await`, :ref:`Generator`, :doc:`../api/coroutines`

co_await
~~~~~~~~

**Definition**: A keyword that suspends a coroutine until an asynchronous operation completes.

**Why LearnQL Uses It**: Enables asynchronous query execution without callback hell.

**Example**:

.. code-block:: cpp

   Task<std::vector<Student>> fetchStudents() {
       auto results = co_await asyncQuery();  // Suspend until query completes
       co_return results;
   }

**See Also**: :ref:`Coroutines`, :ref:`co_yield`, :doc:`../api/coroutines`

co_yield
~~~~~~~~

**Definition**: A keyword that suspends a coroutine and returns a value to the caller. Used in generator functions.

**Why LearnQL Uses It**: Enables lazy evaluation and streaming of large datasets.

**Example**:

.. code-block:: cpp

   Generator<int> fibonacci() {
       int a = 0, b = 1;
       while (true) {
           co_yield a;  // Return value and suspend
           auto next = a + b;
           a = b;
           b = next;
       }
   }

   // In LearnQL
   Generator<Student> queryStudents(Table<Student>& table) {
       for (const auto& student : table.scan()) {
           if (student.gpa() > 3.5) {
               co_yield student;  // Stream matching records
           }
       }
   }

**See Also**: :ref:`Coroutines`, :ref:`co_await`, :ref:`Generator`

CTAD (Class Template Argument Deduction)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Definition**: Automatic deduction of template arguments from constructor arguments (introduced in C++17, enhanced in C++20).

**Why LearnQL Uses It**: Reduces boilerplate when creating template instances.

**Example**:

.. code-block:: cpp

   // Before C++17
   std::pair<int, std::string> p1(42, "hello");

   // With CTAD (C++17+)
   std::pair p2(42, "hello");  // Types deduced automatically

   // In LearnQL
   std::vector v{1, 2, 3, 4, 5};  // std::vector<int> deduced

   // Custom class
   template<typename T>
   struct Wrapper {
       T value;
       Wrapper(T v) : value(v) {}
   };

   Wrapper w(42);  // Wrapper<int> deduced

**See Also**: :ref:`auto`, :ref:`decltype`

decltype
~~~~~~~~

**Definition**: An operator that returns the type of an expression without evaluating it.

**Why LearnQL Uses It**: Essential for perfect forwarding, expression templates, and type deduction.

**Example**:

.. code-block:: cpp

   int x = 42;
   decltype(x) y = 10;  // y has type int

   std::vector<int> vec;
   decltype(vec.size()) count = 0;  // count has type size_t

   // In LearnQL expression templates
   template<typename T>
   auto operator==(const Field<T>& field, const T& value) {
       return BinaryExpression<decltype(field), T, Operator::Equal>(field, value);
   }

**See Also**: :ref:`auto`, :ref:`declval`

declval
~~~~~~~

**Definition**: A utility that creates a reference to a type without constructing it. Used in unevaluated contexts like ``decltype`` and ``sizeof``.

**Why LearnQL Uses It**: Enables type introspection without requiring default constructors.

**Example**:

.. code-block:: cpp

   // Check if type has serialize() method
   template<typename T>
   concept HasSerialize = requires(T obj) {
       { std::declval<T>().serialize() };  // Doesn't construct T
   };

   // Get return type without calling function
   using ReturnType = decltype(std::declval<Student>().serialize());

   // In LearnQL
   template<typename T>
   constexpr auto getFieldType() {
       return decltype(std::declval<T>().get_field())();
   }

**See Also**: :ref:`decltype`, :ref:`Concepts`

Fold Expressions
~~~~~~~~~~~~~~~~

**Definition**: A concise syntax for applying binary operators to parameter packs.

**Why LearnQL Uses It**: Simplifies variadic template code, especially for combining multiple conditions.

**Example**:

.. code-block:: cpp

   // Sum all arguments
   template<typename... Args>
   auto sum(Args... args) {
       return (args + ...);  // Unary right fold
   }

   sum(1, 2, 3, 4);  // Returns 10

   // Check if all values are positive
   template<typename... Args>
   bool all_positive(Args... args) {
       return ((args > 0) && ...);  // Fold with &&
   }

   // In LearnQL query DSL
   template<typename... Conditions>
   auto combineConditions(Conditions... conds) {
       return (conds && ...);  // Combine all conditions with AND
   }

**Fold Types**:

- Unary right: ``(pack op ...)`` → ``pack₁ op (pack₂ op (pack₃ op pack₄))``
- Unary left: ``(... op pack)`` → ``((pack₁ op pack₂) op pack₃) op pack₄``
- Binary right: ``(pack op ... op init)``
- Binary left: ``(init op ... op pack)``

**See Also**: :ref:`Parameter packs`, :ref:`Variadic templates`

if constexpr
~~~~~~~~~~~~

**Definition**: A compile-time if statement. Code in the non-taken branch is not compiled.

**Why LearnQL Uses It**: Enables different code paths for different types without runtime cost.

**Example**:

.. code-block:: cpp

   template<typename T>
   void process(T value) {
       if constexpr (std::is_integral_v<T>) {
           // This code only exists for integer types
           std::cout << "Integer: " << value << '\n';
       } else if constexpr (std::is_floating_point_v<T>) {
           // This code only exists for floating-point types
           std::cout << "Float: " << value << '\n';
       } else {
           // Other types
           std::cout << "Other\n";
       }
   }

   // In LearnQL serialization
   template<typename T>
   void serialize(const T& value) {
       if constexpr (std::is_same_v<T, std::string>) {
           serializeString(value);
       } else if constexpr (std::is_arithmetic_v<T>) {
           serializeNumber(value);
       }
   }

**See Also**: :doc:`../architecture/reflection-system`

Lambda Expressions
~~~~~~~~~~~~~~~~~~

**Definition**: Anonymous function objects that can capture variables from their surrounding scope.

**Why LearnQL Uses It**: Provides concise syntax for callbacks, predicates, and custom operations.

**Example**:

.. code-block:: cpp

   // Basic lambda
   auto add = [](int a, int b) { return a + b; };

   // Capture by value
   int multiplier = 10;
   auto multiply = [multiplier](int x) { return x * multiplier; };

   // Capture by reference
   int count = 0;
   auto increment = [&count]() { ++count; };

   // In LearnQL
   auto results = students.query()
       .where([](const Student& s) { return s.gpa() > 3.5; })
       .collect();

   // Aggregation with lambda
   auto avg_gpa = students.query()
       .aggregate([](const auto& group) {
           return std::accumulate(group.begin(), group.end(), 0.0,
               [](double sum, const auto& s) { return sum + s.gpa(); }) / group.size();
       });

**Capture Modes**:

- ``[]``: Capture nothing
- ``[=]``: Capture all by value
- ``[&]``: Capture all by reference
- ``[x]``: Capture x by value
- ``[&x]``: Capture x by reference
- ``[=, &x]``: Capture all by value except x by reference

**See Also**: :doc:`../tutorials/tutorial-03-query-dsl`

Move Semantics
~~~~~~~~~~~~~~

**Definition**: A C++11+ feature that allows transferring resources from one object to another instead of copying.

**Why LearnQL Uses It**: Improves performance by avoiding expensive copies of large objects.

**Example**:

.. code-block:: cpp

   std::string source = "Hello";
   std::string dest = std::move(source);  // Transfer ownership
   // source is now in valid but unspecified state

   // In LearnQL
   std::vector<Student> results = students.query()
       .where(Student::age > 18)
       .collect();  // Results moved, not copied

   // Move semantics in your classes
   class Student {
       std::string name_;
   public:
       // Move constructor
       Student(Student&& other) noexcept
           : name_(std::move(other.name_)) {}

       // Move assignment
       Student& operator=(Student&& other) noexcept {
           name_ = std::move(other.name_);
           return *this;
       }
   };

**See Also**: :ref:`std::forward`, :ref:`Perfect forwarding`, :ref:`RVO/NRVO`

Parameter Packs
~~~~~~~~~~~~~~~

**Definition**: A template parameter that accepts zero or more template arguments.

**Why LearnQL Uses It**: Enables functions that work with any number of arguments.

**Example**:

.. code-block:: cpp

   // Function template with parameter pack
   template<typename... Args>
   void print(Args... args) {
       (std::cout << ... << args) << '\n';  // Fold expression
   }

   print(1, 2, 3, "hello");  // Args = {int, int, int, const char*}

   // In LearnQL
   template<typename... Fields>
   auto select(Fields... fields) {
       return SelectQuery<Fields...>(fields...);
   }

**See Also**: :ref:`Variadic templates`, :ref:`Fold expressions`

Perfect Forwarding
~~~~~~~~~~~~~~~~~~

**Definition**: A technique to forward arguments to another function while preserving their value category (lvalue/rvalue).

**Why LearnQL Uses It**: Ensures optimal performance when passing arguments through wrapper functions.

**Example**:

.. code-block:: cpp

   // Without perfect forwarding
   template<typename T>
   void wrapper_bad(T arg) {
       process(arg);  // Always copies
   }

   // With perfect forwarding
   template<typename T>
   void wrapper_good(T&& arg) {
       process(std::forward<T>(arg));  // Preserves lvalue/rvalue
   }

   // In LearnQL
   template<typename T, typename... Args>
   void insert(Args&&... args) {
       table_.emplace(std::forward<Args>(args)...);
   }

**See Also**: :ref:`std::forward`, :ref:`Move semantics`

Ranges
~~~~~~

**Definition**: C++20's unified framework for working with sequences of elements. Ranges are composable, lazy, and powerful.

**Why LearnQL Uses It**: Enables elegant query composition and integration with standard algorithms.

**Example**:

.. code-block:: cpp

   #include <ranges>
   namespace views = std::views;

   std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

   // Composable pipeline
   auto result = numbers
       | views::filter([](int n) { return n % 2 == 0; })  // Even numbers
       | views::transform([](int n) { return n * n; })     // Square them
       | views::take(3);                                    // First 3

   // In LearnQL
   auto top_students = students.query()
       .where(Student::gpa > 3.5)
       .collect()
       | views::filter([](const auto& s) { return s.age() < 25; })
       | views::take(10);

**See Also**: :ref:`Views`, :doc:`../api/ranges`

Requires Clause
~~~~~~~~~~~~~~~

**Definition**: A clause that specifies constraints on template parameters using the ``requires`` keyword.

**Why LearnQL Uses It**: Enforces type requirements at compile-time with clear error messages.

**Example**:

.. code-block:: cpp

   // Trailing requires clause
   template<typename T>
   void process(T value) requires std::is_integral_v<T> {
       // Only works with integer types
   }

   // Compound requires clause
   template<typename T>
       requires Serializable<T> && Reflectable<T>
   class Table {
       // T must satisfy both concepts
   };

   // Requires expression
   template<typename T>
   concept Comparable = requires(T a, T b) {
       { a < b } -> std::convertible_to<bool>;
       { a > b } -> std::convertible_to<bool>;
   };

**See Also**: :ref:`Concepts`, :ref:`Constraints`

RVO/NRVO
~~~~~~~~

**Definition**: Return Value Optimization and Named Return Value Optimization. Compiler optimizations that eliminate unnecessary copies when returning objects.

**Why LearnQL Uses It**: Ensures queries return large result sets efficiently without copies.

**Example**:

.. code-block:: cpp

   // RVO (Return Value Optimization)
   std::vector<int> createVector() {
       return std::vector<int>{1, 2, 3};  // No copy, direct construction
   }

   // NRVO (Named Return Value Optimization)
   std::vector<int> processData() {
       std::vector<int> result;
       result.push_back(1);
       return result;  // No copy if compiler applies NRVO
   }

   // In LearnQL
   std::vector<Student> query() {
       std::vector<Student> results;
       // ... populate results
       return results;  // Optimized away
   }

**Note**: In C++17+, RVO is guaranteed for prvalues. NRVO is still optional but widely implemented.

**See Also**: :ref:`Move semantics`

std::any
~~~~~~~~

**Definition**: A type-safe container that can hold a value of any type.

**Why LearnQL Uses It**: Useful for dynamic field values in generic database operations.

**Example**:

.. code-block:: cpp

   #include <any>

   std::any value = 42;
   value = std::string("hello");
   value = 3.14;

   // Access with type checking
   if (value.type() == typeid(std::string)) {
       std::string str = std::any_cast<std::string>(value);
   }

   // In LearnQL (could be used for dynamic queries)
   std::map<std::string, std::any> dynamic_record;
   dynamic_record["id"] = 1;
   dynamic_record["name"] = std::string("Alice");

**See Also**: :ref:`std::variant`, :ref:`std::optional`

std::forward
~~~~~~~~~~~~

**Definition**: A utility function that forwards an argument while preserving its value category.

**Why LearnQL Uses It**: Essential for implementing perfect forwarding in template functions.

**Example**:

.. code-block:: cpp

   // Perfect forwarding wrapper
   template<typename T, typename... Args>
   std::unique_ptr<T> make_unique(Args&&... args) {
       return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
   }

   // In LearnQL
   template<typename T, typename... Args>
   RecordId insert(Args&&... args) {
       T record(std::forward<Args>(args)...);
       return insertRecord(std::move(record));
   }

**See Also**: :ref:`Perfect forwarding`, :ref:`Move semantics`

std::optional
~~~~~~~~~~~~~

**Definition**: A wrapper that may or may not contain a value. Represents optional values without using pointers.

**Why LearnQL Uses It**: Represents nullable fields and query results that may not exist.

**Example**:

.. code-block:: cpp

   #include <optional>

   std::optional<int> findValue(int key) {
       if (/* found */) {
           return 42;
       }
       return std::nullopt;  // No value
   }

   // Usage
   auto result = findValue(10);
   if (result.has_value()) {
       std::cout << "Found: " << *result << '\n';
   }

   // In LearnQL
   std::optional<Student> find(int id) {
       if (auto it = index_.find(id); it != index_.end()) {
           return *it;
       }
       return std::nullopt;
   }

**See Also**: :ref:`std::variant`, :ref:`std::any`

std::span
~~~~~~~~~

**Definition**: A non-owning view over a contiguous sequence of objects. Provides array-like access without copying.

**Why LearnQL Uses It**: Efficient, safe access to memory buffers in serialization and storage.

**Example**:

.. code-block:: cpp

   #include <span>

   void process(std::span<int> data) {
       for (int value : data) {
           std::cout << value << '\n';
       }
   }

   std::vector<int> vec = {1, 2, 3, 4, 5};
   process(vec);  // No copy, just a view

   int arr[] = {1, 2, 3};
   process(arr);  // Works with C arrays too

   // In LearnQL storage
   void writePage(std::span<const std::byte> data) {
       file_.write(data.data(), data.size());
   }

**See Also**: :doc:`../api/serialization`, :doc:`../architecture/storage-engine`

std::variant
~~~~~~~~~~~~

**Definition**: A type-safe union that can hold one of several alternative types.

**Why LearnQL Uses It**: Represents query operators and expressions that can have different types.

**Example**:

.. code-block:: cpp

   #include <variant>

   std::variant<int, double, std::string> value;

   value = 42;              // Holds int
   value = 3.14;            // Now holds double
   value = "hello";         // Now holds string

   // Access with visitor
   std::visit([](auto&& arg) {
       using T = std::decay_t<decltype(arg)>;
       if constexpr (std::is_same_v<T, int>) {
           std::cout << "int: " << arg << '\n';
       } else if constexpr (std::is_same_v<T, double>) {
           std::cout << "double: " << arg << '\n';
       }
   }, value);

   // In LearnQL expression templates
   using Operator = std::variant<EqualOp, LessThanOp, GreaterThanOp>;

**See Also**: :ref:`std::optional`, :ref:`std::any`, :ref:`if constexpr`

Structured Bindings
~~~~~~~~~~~~~~~~~~~

**Definition**: A feature that allows unpacking members of a struct, tuple, or array into named variables.

**Why LearnQL Uses It**: Makes code more readable when working with tuples and pairs.

**Example**:

.. code-block:: cpp

   // With pair/tuple
   std::pair<int, std::string> getPerson() {
       return {1, "Alice"};
   }

   auto [id, name] = getPerson();  // Unpack into separate variables

   // With struct
   struct Point { int x, y; };
   Point p{10, 20};
   auto [x, y] = p;

   // In LearnQL
   for (const auto& [key, value] : index_map_) {
       // key and value unpacked from map entry
   }

   auto [avg_gpa, count] = students.query()
       .aggregate([](const auto& group) {
           return std::make_pair(calculate_avg(group), group.size());
       });

**See Also**: :doc:`../tutorials/tutorial-05-aggregations-groupby`

Variadic Templates
~~~~~~~~~~~~~~~~~~

**Definition**: Templates that accept a variable number of template arguments.

**Why LearnQL Uses It**: Enables flexible APIs that work with any number of fields or arguments.

**Example**:

.. code-block:: cpp

   // Variadic function template
   template<typename... Args>
   void printAll(Args... args) {
       (std::cout << ... << args) << '\n';
   }

   printAll(1, 2, "hello", 3.14);

   // Variadic class template
   template<typename... Fields>
   class Record {
       std::tuple<Fields...> data_;
   };

   // In LearnQL
   template<typename... Fields>
   auto select(Fields... fields) {
       return Query<Fields...>{fields...};
   }

**See Also**: :ref:`Parameter packs`, :ref:`Fold expressions`

Views
~~~~~

**Definition**: Lightweight range adaptors that provide a view over a range without copying elements. Views are lazy and composable.

**Why LearnQL Uses It**: Enables efficient query composition and transformations.

**Example**:

.. code-block:: cpp

   #include <ranges>
   namespace views = std::views;

   std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

   // Views are lazy - no computation yet
   auto even_squares = numbers
       | views::filter([](int n) { return n % 2 == 0; })
       | views::transform([](int n) { return n * n; });

   // Computation happens during iteration
   for (int value : even_squares) {
       std::cout << value << '\n';  // 4, 16, 36, 64, 100
   }

   // In LearnQL
   auto results = students.query()
       .where(Student::department == "CS")
       .collect()
       | views::take(10);

**Common Views**:

- ``views::filter``: Select elements matching predicate
- ``views::transform``: Apply function to each element
- ``views::take``: First N elements
- ``views::drop``: Skip first N elements
- ``views::reverse``: Reverse order

**See Also**: :ref:`Ranges`, :doc:`../api/ranges`

Advanced Template Techniques
-----------------------------

CRTP (Curiously Recurring Template Pattern)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Definition**: A pattern where a class inherits from a template base class, passing itself as a template argument.

**Why LearnQL Uses It**: Enables static polymorphism and compile-time interface implementation.

**Example**:

.. code-block:: cpp

   // Base class uses derived type as template parameter
   template<typename Derived>
   class Base {
   public:
       void interface() {
           static_cast<Derived*>(this)->implementation();
       }
   };

   class Derived : public Base<Derived> {
   public:
       void implementation() {
           std::cout << "Derived implementation\n";
       }
   };

   // In LearnQL
   template<typename Derived>
   class Expression {
       auto evaluate() const {
           return static_cast<const Derived*>(this)->evaluateImpl();
       }
   };

   class BinaryExpression : public Expression<BinaryExpression> {
       auto evaluateImpl() const { /* ... */ }
   };

**Benefits**: No virtual function overhead, type-safe compile-time polymorphism.

**See Also**: :doc:`../architecture/expression-templates`, :ref:`Template metaprogramming`

Expression Templates
~~~~~~~~~~~~~~~~~~~~

**Definition**: A technique that uses template metaprogramming to build and optimize expression trees at compile-time.

**Why LearnQL Uses It**: Provides SQL-like syntax while maintaining type safety and performance.

**Example**:

.. code-block:: cpp

   // Expression objects represent operations, not results
   auto expr = Student::age > 18 && Student::gpa > 3.5;
   // expr is an expression tree, not a boolean

   // Evaluation happens later
   for (const auto& student : students) {
       if (expr.evaluate(student)) {
           // ...
       }
   }

   // In LearnQL query DSL
   auto query = students.query()
       .where((Student::department == "CS" || Student::department == "Math")
              && Student::age >= 18);

   // The entire expression is built at compile-time
   // No runtime overhead for query construction

**How It Works**:

.. code-block:: cpp

   template<typename Left, typename Right, Operator Op>
   struct BinaryExpression {
       Left left_;
       Right right_;

       template<typename Record>
       bool evaluate(const Record& record) const {
           if constexpr (Op == Operator::And) {
               return left_.evaluate(record) && right_.evaluate(record);
           } else if constexpr (Op == Operator::Or) {
               return left_.evaluate(record) || right_.evaluate(record);
           }
       }
   };

**See Also**: :doc:`../architecture/expression-templates`, :doc:`../tutorials/tutorial-03-query-dsl`

RAII (Resource Acquisition Is Initialization)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Definition**: A C++ idiom where resource lifetime is bound to object lifetime. Resources are acquired in constructors and released in destructors.

**Why LearnQL Uses It**: Ensures automatic cleanup of database resources (files, memory, locks).

**Example**:

.. code-block:: cpp

   class File {
       std::FILE* file_;
   public:
       File(const std::string& path) {
           file_ = std::fopen(path.c_str(), "rb");
           if (!file_) throw std::runtime_error("Cannot open file");
       }

       ~File() {
           if (file_) {
               std::fclose(file_);  // Automatic cleanup
           }
       }

       // Delete copy, allow move
       File(const File&) = delete;
       File(File&& other) noexcept : file_(other.file_) {
           other.file_ = nullptr;
       }
   };

   void process() {
       File f("data.db");
       // Use file...
       // Automatic close when f goes out of scope
   }

   // In LearnQL
   class Database {
       std::unique_ptr<Storage> storage_;
   public:
       Database(const std::string& path)
           : storage_(std::make_unique<Storage>(path)) {
           // Storage acquired
       }

       ~Database() {
           // Storage automatically cleaned up
       }
   };

**See Also**: :doc:`../getting-started/best-practices`, :doc:`../architecture/storage-engine`

SFINAE (Substitution Failure Is Not An Error)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Definition**: A C++ principle where template substitution failures don't cause compilation errors, they just remove that overload from consideration.

**Why LearnQL Uses It**: Enables compile-time type introspection and conditional compilation (largely replaced by concepts in C++20).

**Example**:

.. code-block:: cpp

   // Before C++20 (SFINAE with enable_if)
   template<typename T>
   typename std::enable_if<std::is_integral<T>::value, T>::type
   process(T value) {
       return value * 2;
   }

   template<typename T>
   typename std::enable_if<std::is_floating_point<T>::value, T>::type
   process(T value) {
       return value * 1.5;
   }

   // C++20 way (Concepts - cleaner)
   template<typename T>
       requires std::integral<T>
   T process(T value) {
       return value * 2;
   }

   template<typename T>
       requires std::floating_point<T>
   T process(T value) {
       return value * 1.5;
   }

**Note**: While LearnQL uses C++20 concepts, understanding SFINAE helps when reading older C++ code.

**See Also**: :ref:`Concepts`, :ref:`Constraints`

Template Metaprogramming
~~~~~~~~~~~~~~~~~~~~~~~~

**Definition**: A technique where templates are used to perform computations and generate code at compile-time.

**Why LearnQL Uses It**: Generates reflection metadata, optimizes queries, and validates types without runtime cost.

**Example**:

.. code-block:: cpp

   // Compile-time factorial
   template<int N>
   struct Factorial {
       static constexpr int value = N * Factorial<N-1>::value;
   };

   template<>
   struct Factorial<0> {
       static constexpr int value = 1;
   };

   constexpr int result = Factorial<5>::value;  // 120 at compile-time

   // In LearnQL - count fields at compile-time
   template<typename T>
   struct FieldCount;

   template<typename... Fields>
   struct FieldCount<PropertyList<Fields...>> {
       static constexpr size_t value = sizeof...(Fields);
   };

   // Type list manipulation
   template<typename... Types>
   struct TypeList {
       static constexpr size_t size = sizeof...(Types);
   };

**Modern C++20 Approach**: Many metaprogramming tasks are now simpler with ``constexpr`` functions:

.. code-block:: cpp

   constexpr int factorial(int n) {
       return n <= 1 ? 1 : n * factorial(n - 1);
   }

   constexpr int result = factorial(5);  // Computed at compile-time

**See Also**: :ref:`constexpr`, :ref:`consteval`, :doc:`../architecture/reflection-system`

Database-Specific Terms
-----------------------

Generator
~~~~~~~~~

**Definition**: A coroutine that yields multiple values over time, enabling lazy evaluation and streaming.

**Why LearnQL Uses It**: Memory-efficient iteration over large result sets without loading everything into memory.

**Example**:

.. code-block:: cpp

   // Generator definition
   template<typename T>
   class Generator {
       // Coroutine promise type and iterator
   };

   // Usage
   Generator<Student> streamStudents(Table<Student>& table) {
       for (const auto& student : table.scan()) {
           co_yield student;  // Yield one at a time
       }
   }

   // Iterate without loading all into memory
   for (const Student& student : streamStudents(students)) {
       processStudent(student);  // Process one at a time
   }

**See Also**: :ref:`Coroutines`, :ref:`co_yield`, :doc:`../api/coroutines`

Quick Reference
---------------

Type Deduction
~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Feature
     - Use Case
   * - ``auto``
     - Automatic type deduction from initializer
   * - ``decltype``
     - Get type of expression without evaluation
   * - ``declval``
     - Reference to type in unevaluated context
   * - CTAD
     - Deduce template arguments from constructor

Compile-Time Evaluation
~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Feature
     - Behavior
   * - ``constexpr``
     - Can run at compile-time or runtime
   * - ``consteval``
     - Must run at compile-time
   * - ``constinit``
     - Must initialize at compile-time (but not constant)
   * - ``if constexpr``
     - Compile-time conditional compilation

Type Constraints
~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Feature
     - Purpose
   * - Concepts
     - Named requirements for template parameters
   * - Requires clause
     - Specify constraints on templates
   * - Constraints
     - Boolean expressions restricting types

Asynchronous Programming
~~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Feature
     - Purpose
   * - Coroutines
     - Suspendable functions
   * - ``co_await``
     - Wait for async operation
   * - ``co_yield``
     - Return value and suspend
   * - ``co_return``
     - Return from coroutine

Additional Resources
--------------------

External Learning Resources
~~~~~~~~~~~~~~~~~~~~~~~~~~~

**C++20 Reference**:
- `cppreference.com <https://en.cppreference.com/w/cpp/20>`_ - Comprehensive C++20 reference
- `C++ Core Guidelines <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines>`_ - Best practices

**Books**:
- "C++20 - The Complete Guide" by Nicolai M. Josuttis
- "Professional C++" by Marc Gregoire
- "Effective Modern C++" by Scott Meyers

**Online Courses**:
- `learncpp.com <https://www.learncpp.com/>`_ - Free C++ tutorial
- CppCon talks on YouTube - Conference presentations

Related Documentation
~~~~~~~~~~~~~~~~~~~~~

- :doc:`../getting-started/core-concepts` - LearnQL fundamentals
- :doc:`../architecture/expression-templates` - How expression templates work
- :doc:`../architecture/reflection-system` - Compile-time reflection in LearnQL
- :doc:`../api/coroutines` - Coroutine API reference
- :doc:`../api/ranges` - Ranges integration
- :doc:`faq` - Frequently asked questions

Next Steps
----------

1. Review :doc:`../getting-started/core-concepts` to understand LearnQL's architecture
2. Follow :doc:`../tutorials/tutorial-01-first-database` to build your first database
3. Study :doc:`../architecture/expression-templates` to understand the query DSL
4. Explore :doc:`examples` for real-world use cases

Need Help?
----------

- Check the :doc:`faq` for common questions
- Review :doc:`examples` for working code
- See :doc:`contributing` to ask questions or report issues

----

**Last Updated**: 2025-11-02
