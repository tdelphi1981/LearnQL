Installation
============

LearnQL is a header-only C++20 library, making it easy to integrate into your projects. This guide will walk you through the installation process and requirements.

Requirements
------------

Compiler Support
~~~~~~~~~~~~~~~~

LearnQL requires a C++20 compliant compiler. The following compilers are supported:

* **GCC 10+** (recommended: GCC 11 or later)
* **Clang 12+** (recommended: Clang 14 or later)
* **MSVC 19.29+** (Visual Studio 2019 16.11 or later)

.. note::
   C++20 support varies between compiler versions. We recommend using the latest stable version of your chosen compiler for the best experience.

Build System
~~~~~~~~~~~~

* **CMake 3.23 or later**

Operating Systems
~~~~~~~~~~~~~~~~~

LearnQL is cross-platform and has been tested on:

* Linux (Ubuntu 20.04+, Fedora 35+)
* macOS (10.15+)
* Windows (Windows 10+)

Installation Methods
--------------------

Method 1: Direct Include (Simplest)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The easiest way to use LearnQL is to copy the ``learnql/`` directory directly into your project:

.. code-block:: bash

   # Clone the repository
   git clone https://github.com/tdelphi1981/LearnQL.git

   # Copy the header-only library to your project
   cp -r LearnQL/learnql /path/to/your/project/include/

Then include it in your C++ code:

.. code-block:: cpp

   #include <learnql/LearnQL.hpp>

Method 2: CMake Integration (Recommended)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Using CMake's FetchContent
^^^^^^^^^^^^^^^^^^^^^^^^^^

Add LearnQL to your ``CMakeLists.txt``:

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.23)
   project(MyProject CXX)

   # Set C++20 standard
   set(CMAKE_CXX_STANDARD 20)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)

   # Fetch LearnQL
   include(FetchContent)
   FetchContent_Declare(
       learnql
       GIT_REPOSITORY https://github.com/tdelphi1981/LearnQL.git
       GIT_TAG main
   )
   FetchContent_MakeAvailable(learnql)

   # Your executable
   add_executable(myapp main.cpp)
   target_link_libraries(myapp PRIVATE learnql)

Using add_subdirectory
^^^^^^^^^^^^^^^^^^^^^^^

If you've cloned LearnQL as a submodule or subdirectory:

.. code-block:: cmake

   # In your CMakeLists.txt
   add_subdirectory(external/LearnQL)

   add_executable(myapp main.cpp)
   target_link_libraries(myapp PRIVATE learnql)

Method 3: System-Wide Installation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Install LearnQL system-wide using CMake:

.. code-block:: bash

   git clone https://github.com/tdelphi1981/LearnQL.git
   cd LearnQL
   mkdir build && cd build
   cmake ..
   sudo cmake --install .

Then in your ``CMakeLists.txt``:

.. code-block:: cmake

   find_package(LearnQL REQUIRED)
   target_link_libraries(myapp PRIVATE LearnQL::learnql)

Verifying Installation
----------------------

Create a simple test program to verify your installation:

.. code-block:: cpp

   // test.cpp
   #include <learnql/LearnQL.hpp>
   #include <iostream>

   using namespace learnql;

   class Person {
       LEARNQL_PROPERTIES_BEGIN(Person)
           LEARNQL_PROPERTY(int, person_id, PK)
           LEARNQL_PROPERTY(std::string, name)
       LEARNQL_PROPERTIES_END(
           PROP(int, person_id, PK),
           PROP(std::string, name)
       )

   public:
       Person() = default;
       Person(int id, const std::string& n) : person_id_(id), name_(n) {}
   };

   int main() {
       std::cout << "LearnQL version: 1.0.0" << std::endl;

       core::Database db(":memory:");
       auto& people = db.table<Person>("people");

       people.insert(Person(1, "Alice"));

       std::cout << "Successfully created database and inserted record!" << std::endl;
       return 0;
   }

Compile and run:

.. code-block:: bash

   # Using g++
   g++ -std=c++20 -I/path/to/learnql test.cpp -o test
   ./test

   # Using CMake
   mkdir build && cd build
   cmake ..
   cmake --build .
   ./test

Expected output:

.. code-block:: text

   LearnQL version: 1.0.0
   Successfully created database and inserted record!

Troubleshooting
---------------

Compiler Errors About C++20 Features
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:** Errors like "concepts are not supported" or "requires clause not recognized"

**Solution:** Ensure you're using a compatible compiler version and that C++20 is enabled:

.. code-block:: bash

   g++ -std=c++20 ...  # For GCC
   clang++ -std=c++20 ... # For Clang

In CMake:

.. code-block:: cmake

   set(CMAKE_CXX_STANDARD 20)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)

Missing Header Files
~~~~~~~~~~~~~~~~~~~~

**Problem:** "LearnQL.hpp: No such file or directory"

**Solution:** Verify the include path is correct. The include should point to the parent directory of ``learnql/``:

.. code-block:: bash

   # Correct structure:
   # your-project/
   #   include/
   #     learnql/
   #       LearnQL.hpp
   #       core/
   #       query/
   #       ...

   g++ -Iinclude ...

Linker Errors
~~~~~~~~~~~~~

**Problem:** Undefined reference errors

**Solution:** LearnQL is header-only and should not produce linker errors. If you encounter them:

1. Ensure all ``.hpp`` files are in the include path
2. Check that you're not trying to link against object files
3. Verify no conflicting library versions

Performance Issues in Debug Mode
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:** Database operations are slow in debug builds

**Solution:** LearnQL uses template-heavy code that benefits significantly from optimization:

.. code-block:: bash

   # For better performance, use release mode
   cmake -DCMAKE_BUILD_TYPE=Release ..

Next Steps
----------

Now that LearnQL is installed, continue with:

* :doc:`quick-start` - Build your first database application in 30 minutes
* :doc:`core-concepts` - Understand LearnQL's fundamental concepts
* :doc:`/tutorials/tutorial-01-first-database` - Follow the comprehensive tutorial series
