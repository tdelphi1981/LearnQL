LearnQL Documentation
=====================

Welcome to LearnQL, a modern, type-safe C++20 database library designed for learning and prototyping!

LearnQL demonstrates advanced C++20 features including concepts, coroutines, ranges, and template metaprogramming through a practical database implementation. Whether you're learning modern C++ or building a prototype application, LearnQL provides an educational and powerful foundation.

.. note::
   LearnQL is designed for **educational purposes and prototyping**. It is not intended for production use.

Key Features
------------

✨ **Modern C++20**
   Built entirely with C++20 features including concepts, coroutines, ranges, and compile-time reflection

🔒 **Type-Safe Queries**
   SQL-like query DSL with full compile-time type checking using expression templates

📦 **Zero Dependencies**
   Header-only library with no external dependencies

💾 **Persistent Storage**
   Page-based storage engine with B-tree indexing

🚀 **Property Macros**
   Automatic code generation that reduces boilerplate by ~70%

🎓 **Educational**
   Comprehensive examples and documentation for learning database internals

Quick Example
-------------

.. code-block:: cpp

   #include <learnql/LearnQL.hpp>
   using namespace learnql;

   class Student {
       LEARNQL_PROPERTIES_BEGIN(Student)
           LEARNQL_PROPERTY(int, student_id, PK)
           LEARNQL_PROPERTY(std::string, name)
           LEARNQL_PROPERTY(int, age)
       LEARNQL_PROPERTIES_END(
           PROP(int, student_id, PK),
           PROP(std::string, name),
           PROP(int, age)
       )
   public:
       Student() = default;
       Student(int id, const std::string& n, int a)
           : student_id_(id), name_(n), age_(a) {}
   };

   int main() {
       core::Database db("university.db");
       auto& students = db.table<Student>("students");

       // Insert a student
       students.insert(Student(1, "Alice", 20));

       // Query with type-safe expressions using static Fields
       auto adults = students.where(Student::age >= 18);

       for (const auto& student : adults) {
           std::cout << student.get_name() << std::endl;
       }
   }

Documentation Structure
-----------------------

.. toctree::
   :maxdepth: 2
   :caption: Getting Started

   getting-started/installation
   getting-started/quick-start
   getting-started/core-concepts
   getting-started/property-macros
   getting-started/best-practices

.. toctree::
   :maxdepth: 2
   :caption: Tutorials

   tutorials/tutorial-01-first-database
   tutorials/tutorial-02-crud-operations
   tutorials/tutorial-03-query-dsl
   tutorials/tutorial-04-joins-relationships
   tutorials/tutorial-05-aggregations-groupby
   tutorials/tutorial-06-indexes-performance
   tutorials/tutorial-07-advanced-features

.. toctree::
   :maxdepth: 2
   :caption: API Reference

   api/core
   api/query
   api/storage
   api/index
   api/catalog
   api/reflection
   api/serialization
   api/ranges
   api/coroutines
   api/debug

.. toctree::
   :maxdepth: 2
   :caption: Architecture

   architecture/overview
   architecture/storage-engine
   architecture/btree-implementation
   architecture/expression-templates
   architecture/reflection-system
   architecture/performance

.. toctree::
   :maxdepth: 2
   :caption: Additional Resources

   resources/cpp20-glossary
   resources/faq
   resources/examples
   resources/contributing

Indices and Tables
==================

* :ref:`genindex`
* :ref:`search`
