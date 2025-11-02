# LearnQL

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/CMake-3.23+-064F8C.svg)](https://cmake.org/)
[![Header-Only](https://img.shields.io/badge/library-header--only-orange.svg)](https://github.com/tolgaberber/LearnQL)
[![License](https://img.shields.io/badge/license-AFL--3.0-green.svg)](LICENSE)

A modern, type-safe C++20 database library featuring expression template queries, compile-time reflection, and persistent storage - designed for learning and prototyping.

## Features

### Core Capabilities
- **Type-Safe Operations** - Compile-time type checking with C++20 concepts
- **SQL-like Query DSL** - Intuitive expression templates for filtering, joining, and grouping
- **Property Macros** - Automatic code generation reducing boilerplate by ~70%
- **Compile-Time Reflection** - Zero-runtime-overhead introspection system
- **Persistent Storage** - Page-based storage engine with automatic serialization
- **Secondary Indexes** - B-tree indexes with batched loading and auto-persistence
- **System Catalog** - Queryable metadata tables for schema introspection
- **C++20 Ranges** - First-class ranges support for composable operations
- **Coroutines** - Async query execution with generator support
- **Debug Tools** - Built-in profiler, statistics, and execution plan visualization

## Quick Start

### Requirements
- C++20 compliant compiler (GCC 10+, Clang 12+, MSVC 19.29+)
- CMake 3.23 or higher

### Installation

LearnQL is a header-only library. Simply include it in your project:

```cmake
# Add LearnQL to your CMakeLists.txt
add_subdirectory(path/to/LearnQL)
target_link_libraries(your_target PRIVATE learnql)
```

Or copy the `learnql/` directory to your include path.

### Basic Example

```cpp
#include <learnql/LearnQL.hpp>

using namespace learnql;

// Define your model with property macros
class Student {
    LEARNQL_PROPERTIES_BEGIN(Student)
        LEARNQL_PROPERTY(int, student_id, PK)
        LEARNQL_PROPERTY(std::string, name)
        LEARNQL_PROPERTY(std::string, department)
        LEARNQL_PROPERTY(int, age)
        LEARNQL_PROPERTY(double, gpa)
    LEARNQL_PROPERTIES_END(
        PROP(int, student_id, PK),
        PROP(std::string, name),
        PROP(std::string, department),
        PROP(int, age),
        PROP(double, gpa)
    )

public:
    Student() = default;
    Student(int id, const std::string& n, const std::string& dept, int a, double g)
        : student_id_(id), name_(n), department_(dept), age_(a), gpa_(g) {}
};

int main() {
    // Create database and table
    core::Database db("university.db");
    auto& students = db.table<Student>("students");

    // Insert records
    students.insert(Student(1, "Alice", "CS", 20, 3.8));
    students.insert(Student(2, "Bob", "Math", 22, 3.5));
    students.insert(Student(3, "Charlie", "CS", 21, 3.9));

    // Query with expression templates
    auto cs_students = students.query()
        .where(Student::department == "CS" && Student::gpa > 3.7)
        .orderBy<&Student::gpa>(false)
        .collect();

    // Range-based iteration
    for (const auto& student : cs_students) {
        std::cout << student.name() << " - GPA: " << student.gpa() << '\n';
    }

    return 0;
}
```

Output:
```
Charlie - GPA: 3.9
Alice - GPA: 3.8
```

## Key Concepts

### Property Macros

LearnQL's property system generates getters, setters, static Field objects, serialization code, and reflection metadata from a single declaration:

```cpp
LEARNQL_PROPERTIES_BEGIN(Student)
    LEARNQL_PROPERTY(int, student_id, PK)  // PK = Primary Key
    LEARNQL_PROPERTY(std::string, name)
LEARNQL_PROPERTIES_END(
    PROP(int, student_id, PK),
    PROP(std::string, name)
)
```

This generates:
- Private member variables: `student_id_`, `name_`
- Getters: `student_id()`, `name()`
- Setters: `set_student_id()`, `set_name()`
- Static fields: `Student::student_id`, `Student::name`
- Serialization methods: `serialize()`, `deserialize()`
- Reflection metadata for system catalog

### Expression Templates

Write queries that look like SQL but are type-checked at compile time:

```cpp
// Complex WHERE conditions
auto results = table.query()
    .where((Student::age >= 18 && Student::age <= 25) || Student::gpa > 3.5)
    .collect();

// Joins
auto enrolled = students.query()
    .innerJoin(enrollments, Student::student_id == Enrollment::student_id)
    .collect();

// Aggregations
auto dept_stats = students.query()
    .groupBy<&Student::department>()
    .aggregate([](const auto& group) {
        return std::make_pair(
            group.key(),
            std::accumulate(group.begin(), group.end(), 0.0,
                [](double sum, const auto& s) { return sum + s.gpa(); }) / group.size()
        );
    })
    .collect();
```

### Secondary Indexes

Create indexes for fast lookups on non-primary-key fields:

```cpp
// Create an index on the department field
students.createIndex<&Student::department>("idx_department");

// Queries automatically use indexes when available
auto cs_students = students.query()
    .where(Student::department == "CS")
    .collect();  // O(log n) instead of O(n)
```

### System Catalog

Introspect database schema at runtime using queryable metadata tables:

```cpp
auto catalog = db.systemCatalog();

// Query table metadata
auto tables = catalog.tables().query()
    .where(catalog::TableMetadata::name.like("%student%"))
    .collect();

// Query field information
auto fields = catalog.fields().query()
    .where(catalog::FieldMetadata::table_name == "students")
    .collect();
```

### C++20 Ranges Integration

Compose queries with standard ranges:

```cpp
auto top_students = students.query()
    .where(Student::gpa > 3.5)
    .collect()
    | std::views::filter([](const auto& s) { return s.age() < 25; })
    | std::views::take(10);
```

### Coroutine Support

Use generators for memory-efficient streaming:

```cpp
coroutines::Generator<Student> streamStudents(core::Table<Student>& table) {
    for (const auto& student : table.scan()) {
        co_yield student;
    }
}

for (const auto& student : streamStudents(students)) {
    // Process one at a time without loading all into memory
}
```

## Advanced Features

### Batched Loading

Load large datasets efficiently:

```cpp
for (const auto& batch : students.scan_batched(1000)) {
    // Process 1000 records at a time
    for (const auto& student : batch) {
        // ...
    }
}
```

### Read-Only Tables

Protect metadata from accidental modification:

```cpp
core::ReadOnlyTable<TableMetadata> tables(storage, "sys_tables");
// tables.insert(...);  // Compile error!
```

### Query Profiling

Analyze query performance:

```cpp
debug::Profiler profiler;
profiler.start("complex_query");

auto results = students.query()
    .where(Student::gpa > 3.5)
    .collect();

profiler.stop("complex_query");
std::cout << profiler.report() << '\n';
```

### Database Inspector

Inspect database structure and statistics:

```cpp
utils::DbInspector inspector(db);
inspector.printDatabaseStructure();
inspector.printTableStatistics("students");
```

## Architecture

```
learnql/
├── core/           # Database, Table, RecordId
├── storage/        # Page-based storage engine
├── query/          # Query DSL and expression templates
├── index/          # B-tree and secondary indexes
├── catalog/        # System catalog for metadata
├── reflection/     # Compile-time reflection
├── meta/           # Property macros and type info
├── serialization/  # Binary serialization
├── ranges/         # C++20 ranges integration
├── coroutines/     # Async query support
├── concepts/       # Type constraints
├── debug/          # Profiling and debugging tools
└── utils/          # Utility functions
```

## Performance Considerations

- **Header-Only**: Zero library linking overhead
- **Compile-Time Reflection**: No runtime reflection cost
- **Index Usage**: Automatic index selection for queries
- **Page-Based Storage**: Efficient disk I/O with configurable page size
- **Batched Operations**: Memory-efficient bulk operations
- **Expression Templates**: Zero-cost abstraction for queries

## Limitations

LearnQL is designed for **learning and prototyping**, not production use:

- Single-threaded (no concurrent access support)
- No transaction support
- Limited to in-process usage
- Not optimized for large-scale datasets (> 1M records)
- No query optimizer
- Basic error handling

## Documentation

- [API Reference](learnql/LearnQL.hpp) - Comprehensive inline documentation
- [Examples](main.cpp) - Complete working example
- [Property Macros Guide](learnql/meta/Property.hpp)
- [Query DSL Guide](learnql/query/Query.hpp)
- [System Catalog Guide](learnql/catalog/SystemCatalog.hpp)

## Building

```bash
# Clone the repository
git clone https://github.com/yourusername/LearnQL.git
cd LearnQL

# Build with CMake
mkdir build && cd build
cmake ..
cmake --build .

# Run the example
./LearnQL
```

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

This project is licensed under the Academic Free License 3.0 - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

LearnQL was created as an educational project to demonstrate modern C++20 features including:
- Concepts and constraints
- Coroutines
- Ranges
- Template metaprogramming
- Expression templates
- Compile-time reflection

## Future Roadmap

- [ ] Multi-threaded query execution
- [ ] Transaction support with ACID guarantees
- [ ] Query optimizer with cost-based planning
- [ ] Network protocol for client-server architecture
- [ ] Additional index types (hash, full-text)
- [ ] Compression support for storage
- [ ] Write-ahead logging (WAL)
- [ ] MVCC for concurrent access

## Contact

For questions, issues, or suggestions:
- Open an issue on GitHub
- Contribute improvements via pull requests

---

**Note**: LearnQL is an educational project. For production databases, consider established solutions like SQLite, PostgreSQL, or RocksDB.
