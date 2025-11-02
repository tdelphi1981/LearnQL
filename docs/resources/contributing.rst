Contributing to LearnQL
=======================

Thank you for your interest in contributing to LearnQL! This guide will help you get started with contributing code, documentation, examples, and more.

.. note::
   LearnQL is an educational project. We welcome contributions from developers of all skill levels, especially beginners learning C++20!

.. contents:: Quick Navigation
   :local:
   :depth: 2

Code of Conduct
---------------

Creating a Welcoming Environment
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

By participating in this project, you agree to maintain a respectful and inclusive environment:

**Our Standards**

- **Be Respectful**: Treat everyone with respect and kindness
- **Be Constructive**: Provide helpful feedback and accept criticism gracefully
- **Be Inclusive**: Welcome newcomers and help them learn
- **Be Patient**: Remember that everyone is learning
- **Be Professional**: Focus on what is best for the community and project

**Unacceptable Behavior**

- Harassment, discrimination, or offensive comments
- Personal attacks or insults
- Trolling or deliberately inflammatory remarks
- Publishing others' private information
- Other conduct that would be inappropriate in a professional setting

**Reporting Issues**

If you experience or witness unacceptable behavior:

1. Contact the project maintainers via email or GitHub
2. Provide as much detail as possible
3. Maintainers will review and respond promptly
4. All complaints will be handled with discretion

**Enforcement**

Maintainers have the right to:

- Remove, edit, or reject comments, commits, and issues
- Ban temporarily or permanently any contributor for behaviors deemed inappropriate
- Take any other action deemed necessary

How to Contribute
-----------------

Types of Contributions
~~~~~~~~~~~~~~~~~~~~~~

We welcome various types of contributions:

**1. Bug Fixes**
   Find and fix issues in the existing code

**2. New Features**
   Implement new functionality or enhance existing features

**3. Documentation**
   Improve or add documentation (tutorials, API docs, examples)

**4. Examples**
   Create example applications demonstrating LearnQL features

**5. Performance Improvements**
   Optimize existing code for better performance

**6. Tests**
   Add or improve test coverage

**7. Code Quality**
   Refactoring and code cleanup

**8. Questions & Discussions**
   Help other users by answering questions

Before You Start
~~~~~~~~~~~~~~~~

**1. Check Existing Issues**

Search the `issue tracker <https://github.com/yourusername/LearnQL/issues>`_ to see if your idea is already being discussed.

**2. Open an Issue First**

For major changes:

- Open an issue describing your proposed change
- Discuss the approach with maintainers
- Wait for feedback before starting work

This prevents duplicate work and ensures your contribution aligns with project goals.

**3. Keep Changes Focused**

- One feature or fix per pull request
- Don't combine unrelated changes
- Break large features into smaller PRs

**4. Follow the Guidelines**

Read this entire document before contributing.

Getting Started
---------------

Prerequisites
~~~~~~~~~~~~~

**Required Tools**:

- **Git**: Version control
- **C++20 Compiler**:

  - GCC 10 or higher
  - Clang 12 or higher
  - MSVC 19.29 (Visual Studio 2019 16.11) or higher

- **CMake**: 3.23 or higher
- **Text Editor or IDE**: Your choice (VSCode, CLion, Visual Studio, etc.)

**Recommended Skills**:

- Basic C++ knowledge
- Familiarity with Git
- Understanding of pull request workflow
- (Optional) C++20 features knowledge

Fork and Clone
~~~~~~~~~~~~~~

**1. Fork the Repository**

Click the "Fork" button on GitHub to create your own copy.

**2. Clone Your Fork**

.. code-block:: bash

   git clone https://github.com/YOUR_USERNAME/LearnQL.git
   cd LearnQL

**3. Add Upstream Remote**

.. code-block:: bash

   git remote add upstream https://github.com/ORIGINAL_OWNER/LearnQL.git

**4. Verify Remotes**

.. code-block:: bash

   git remote -v
   # origin    https://github.com/YOUR_USERNAME/LearnQL.git (fetch)
   # origin    https://github.com/YOUR_USERNAME/LearnQL.git (push)
   # upstream  https://github.com/ORIGINAL_OWNER/LearnQL.git (fetch)
   # upstream  https://github.com/ORIGINAL_OWNER/LearnQL.git (push)

Development Setup
-----------------

Building the Project
~~~~~~~~~~~~~~~~~~~~

**1. Create Build Directory**

.. code-block:: bash

   mkdir build && cd build

**2. Configure with CMake**

.. code-block:: bash

   # Debug build (for development)
   cmake -DCMAKE_BUILD_TYPE=Debug ..

   # Release build (for performance testing)
   cmake -DCMAKE_BUILD_TYPE=Release ..

**3. Build**

.. code-block:: bash

   cmake --build .

**4. Run the Example**

.. code-block:: bash

   ./LearnQL

IDE Setup
~~~~~~~~~

**CLion**

1. Open the project directory in CLion
2. CLion will automatically detect CMake configuration
3. Select your compiler in Settings → Build, Execution, Deployment → Toolchains
4. Build and run using the toolbar

**Visual Studio Code**

1. Install extensions:

   - C/C++ (Microsoft)
   - CMake Tools (Microsoft)

2. Open the project folder
3. Select a kit (compiler) when prompted
4. Use the CMake Tools panel to build and run

**Visual Studio 2019+**

1. Use "Open Folder" to open the project directory
2. Visual Studio will detect the CMake configuration
3. Select a configuration (Debug/Release)
4. Build and run using the toolbar

**Vim/Emacs/Other**

Use command-line CMake as shown above.

Coding Standards
----------------

Style Guidelines
~~~~~~~~~~~~~~~~

LearnQL follows modern C++ best practices. Consistency is key!

General Principles
^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // ✓ DO: Use C++20 features where appropriate
   auto students = db.table<Student>("students");

   // ✓ DO: Follow RAII principles
   class Database {
       std::unique_ptr<Storage> storage_;  // Automatic cleanup
   };

   // ✓ DO: Prefer const correctness
   void process(const Student& student) const;

   // ✓ DO: Use auto when type is obvious
   auto result = calculateAverage(scores);

   // ✗ DON'T: Use raw pointers
   Student* student = new Student();  // NO!

   // ✓ DO: Use smart pointers or values
   auto student = std::make_unique<Student>();
   Student student;  // Or stack allocation

Naming Conventions
^^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // Classes: PascalCase
   class StudentRecord { };
   class BTreeNode { };

   // Functions/Methods: camelCase
   void calculateAverage() { }
   void insertRecord() { }

   // Variables: snake_case
   int student_count = 0;
   std::string table_name = "students";

   // Constants: UPPER_SNAKE_CASE
   constexpr int MAX_PAGE_SIZE = 4096;
   constexpr size_t DEFAULT_BUFFER_SIZE = 1024;

   // Private members: trailing underscore
   class Example {
       int value_;
       std::string name_;
   public:
       int value() const { return value_; }
   };

   // Namespaces: lowercase
   namespace learnql::core { }
   namespace learnql::query { }

   // Template parameters: PascalCase
   template<typename RecordType, typename KeyType>
   class Index { };

   // Concepts: PascalCase
   template<typename T>
   concept Serializable = /* ... */;

File Organization
^^^^^^^^^^^^^^^^^

.. code-block:: cpp

   // 1. Header guard or #pragma once
   #ifndef LEARNQL_CORE_TABLE_HPP
   #define LEARNQL_CORE_TABLE_HPP

   // OR (preferred)
   #pragma once

   // 2. System includes (alphabetical)
   #include <memory>
   #include <string>
   #include <vector>

   // 3. External library includes
   // (LearnQL has none - it's standalone!)

   // 4. Project includes (alphabetical)
   #include "learnql/core/Database.hpp"
   #include "learnql/storage/Storage.hpp"

   // 5. Namespace
   namespace learnql::core {

   // 6. Forward declarations (if needed)
   class Database;

   // 7. Class/function definitions
   template<typename T>
   class Table {
       // Implementation
   };

   } // namespace learnql::core

   #endif // LEARNQL_CORE_TABLE_HPP

Code Formatting
^^^^^^^^^^^^^^^

.. code-block:: cpp

   // Indentation: 4 spaces (no tabs)
   void function() {
       if (condition) {
           doSomething();
       }
   }

   // Line length: Maximum 100 characters (prefer 80)
   void shortFunction() { /* ... */ }

   void longFunctionWithManyParameters(
       const std::string& param1,
       int param2,
       double param3
   ) {
       // Implementation
   }

   // Braces: Opening brace on same line
   if (condition) {
       // code
   } else {
       // code
   }

   // Single-line if: braces optional but recommended
   if (condition) {
       doSomething();  // Preferred
   }

   if (condition)
       doSomething();  // Acceptable

   // Pointer/reference alignment: Attach to type
   int* ptr;           // Preferred
   std::string& ref;   // Preferred
   const char* str;    // Preferred

   // NOT:
   // int *ptr;         // Discouraged
   // int * ptr;        // Discouraged

   // Spacing
   void function(int a, int b) {  // Space after commas
       int c = a + b;             // Spaces around operators
       if (c > 0) {               // Space before opening brace
           doSomething();
       }
   }

Header-Only Library Guidelines
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Since LearnQL is header-only:

.. code-block:: cpp

   // ✓ DO: Use inline for non-template functions
   inline int calculateSum(int a, int b) {
       return a + b;
   }

   // ✓ DO: Templates don't need inline (implicitly inline)
   template<typename T>
   void process(T value) {
       // Implementation
   }

   // ✓ DO: Minimize includes in public headers
   #pragma once

   // Forward declare when possible
   namespace std {
       template<typename T> class vector;
   }

   // Only include what's necessary
   #include <memory>  // For unique_ptr

   // ✓ DO: Be mindful of compile times
   // Avoid including heavy headers unnecessarily

   // ✗ DON'T: Put large implementations in headers without inline
   void largeFunction() {  // Missing inline!
       // 100 lines of code
   }

Documentation Standards
~~~~~~~~~~~~~~~~~~~~~~~

**Doxygen Comments**

Document all public APIs:

.. code-block:: cpp

   /**
    * @brief Inserts a record into the table
    *
    * This function inserts a new record into the table and returns
    * its RecordId. The record is serialized and written to persistent
    * storage immediately.
    *
    * @param record The record to insert
    * @return RecordId of the inserted record
    *
    * @throws std::runtime_error If serialization fails
    * @throws StorageException If disk write fails
    *
    * @code
    * Student student(1, "Alice", "CS", 20, 3.8);
    * RecordId id = students.insert(student);
    * @endcode
    *
    * @note The record must satisfy the Serializable concept
    * @see update(), remove(), findByKey()
    */
   template<typename T>
       requires Serializable<T>
   RecordId insert(const T& record);

**Inline Comments**

Explain **why**, not **what**:

.. code-block:: cpp

   // ✓ GOOD: Explains reasoning
   // Use binary search since the index is sorted by key
   auto it = std::lower_bound(index.begin(), index.end(), key);

   // Use exponential backoff to avoid overwhelming the disk
   std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay));
   retry_delay *= 2;

   // ✗ BAD: Explains what (obvious from code)
   // Increment the counter
   counter++;

   // Set the name to "Alice"
   name = "Alice";

**File Headers**

.. code-block:: cpp

   /**
    * @file Table.hpp
    * @brief Core table implementation
    * @author LearnQL Contributors
    *
    * This file contains the Table class, which provides
    * high-level database operations including CRUD,
    * querying, and index management.
    */

Error Handling
~~~~~~~~~~~~~~

.. code-block:: cpp

   // ✓ DO: Use exceptions for exceptional conditions
   if (page_id >= total_pages_) {
       throw std::out_of_range(
           "Page ID " + std::to_string(page_id) +
           " exceeds total pages " + std::to_string(total_pages_)
       );
   }

   // ✓ DO: Provide clear error messages
   if (!file.is_open()) {
       throw std::runtime_error(
           "Cannot open database file: " + path +
           " (Error: " + std::strerror(errno) + ")"
       );
   }

   // ✓ DO: Document exceptions
   /**
    * @throws std::out_of_range If index is invalid
    * @throws SerializationError If deserialization fails
    */

   // ✓ DO: Use standard exception types when appropriate
   throw std::invalid_argument("Student ID must be positive");
   throw std::logic_error("Table has no primary key");

Testing Guidelines
------------------

Writing Tests
~~~~~~~~~~~~~

While LearnQL doesn't currently have a formal test suite, contributors should thoroughly test their changes.

**Manual Testing Checklist**

Before submitting a PR:

.. code-block:: text

   [ ] Code compiles without warnings on GCC
   [ ] Code compiles without warnings on Clang
   [ ] Code compiles without warnings on MSVC
   [ ] All features work as documented
   [ ] Edge cases tested (empty input, null values, etc.)
   [ ] Error handling works correctly
   [ ] No memory leaks (checked with valgrind/sanitizers)
   [ ] Performance is acceptable
   [ ] Documentation is accurate and complete

Testing Scenarios
^^^^^^^^^^^^^^^^^

Test these scenarios for your changes:

.. code-block:: cpp

   // 1. Normal cases
   students.insert(Student(1, "Alice", "CS", 20, 3.8));

   // 2. Edge cases
   students.insert(Student(0, "", "", 0, 0.0));  // Empty data
   auto results = students.query().where(/* impossible condition */).collect();  // Empty results

   // 3. Error cases
   try {
       students.insert(duplicate_student);  // Duplicate primary key
   } catch (const std::exception& e) {
       // Verify exception is thrown
   }

   // 4. Performance
   for (int i = 0; i < 10000; ++i) {
       students.insert(generateStudent(i));  // Large dataset
   }

   // 5. Cleanup
   db.~Database();  // Ensure proper cleanup

Building with Sanitizers
~~~~~~~~~~~~~~~~~~~~~~~~~

**Address Sanitizer** (detects memory errors):

.. code-block:: bash

   mkdir build-asan && cd build-asan
   cmake -DCMAKE_CXX_FLAGS="-fsanitize=address -g" ..
   cmake --build .
   ./LearnQL

**Undefined Behavior Sanitizer**:

.. code-block:: bash

   mkdir build-ubsan && cd build-ubsan
   cmake -DCMAKE_CXX_FLAGS="-fsanitize=undefined -g" ..
   cmake --build .
   ./LearnQL

**Valgrind** (Linux only):

.. code-block:: bash

   valgrind --leak-check=full --show-leak-kinds=all ./LearnQL

Compiler Warnings
~~~~~~~~~~~~~~~~~

Enable all warnings:

.. code-block:: bash

   # GCC/Clang
   cmake -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Werror" ..

   # MSVC
   cmake -DCMAKE_CXX_FLAGS="/W4 /WX" ..

Pull Request Process
--------------------

Creating a Pull Request
~~~~~~~~~~~~~~~~~~~~~~~

**1. Update Your Fork**

.. code-block:: bash

   git fetch upstream
   git checkout main
   git merge upstream/main

**2. Create a Feature Branch**

.. code-block:: bash

   git checkout -b feature/your-feature-name

   # Branch naming conventions:
   # feature/add-transaction-support
   # fix/crash-on-empty-query
   # docs/improve-tutorial-01
   # refactor/simplify-btree-insert

**3. Make Your Changes**

- Follow coding standards
- Add documentation
- Test thoroughly
- Keep commits atomic and focused

**4. Commit Your Changes**

.. code-block:: bash

   git add .
   git commit -m "feat: Add transaction support"

See :ref:`Commit Message Guidelines` below.

**5. Push to Your Fork**

.. code-block:: bash

   git push origin feature/your-feature-name

**6. Open a Pull Request**

1. Go to your fork on GitHub
2. Click "New Pull Request"
3. Select your feature branch
4. Fill out the PR template (see below)
5. Submit the PR

Commit Message Guidelines
~~~~~~~~~~~~~~~~~~~~~~~~~

Follow the `Conventional Commits <https://www.conventionalcommits.org/>`_ format:

**Format**:

.. code-block:: text

   type: Brief summary (50 chars or less)

   More detailed explanation if needed. Wrap at 72 characters.
   Explain the problem this commit solves and how.

   - Bullet points are okay
   - Use present tense: "Add feature" not "Added feature"
   - Reference issues: "Fixes #123" or "Closes #456"

**Types**:

- ``feat``: New feature
- ``fix``: Bug fix
- ``docs``: Documentation changes
- ``style``: Formatting, no code change
- ``refactor``: Code restructuring
- ``perf``: Performance improvement
- ``test``: Adding tests
- ``chore``: Maintenance tasks
- ``build``: Build system changes
- ``ci``: CI/CD changes

**Examples**:

.. code-block:: text

   feat: Add support for secondary indexes

   Implements B-tree based secondary indexes for non-primary-key fields.
   Indexes can be created dynamically and are automatically persisted.

   - Added Index class template
   - Integrated with query optimizer
   - Updated documentation

   Closes #42

.. code-block:: text

   fix: Prevent crash on empty query results

   The query executor crashed when a WHERE clause matched no records.
   Added null check before dereferencing result iterator.

   Fixes #128

.. code-block:: text

   docs: Improve tutorial 01 with more examples

   Added code examples for CRUD operations and expanded the
   explanation of property macros.

Pull Request Template
~~~~~~~~~~~~~~~~~~~~~~

When opening a PR, include:

.. code-block:: markdown

   ## Description
   Brief description of changes

   ## Type of Change
   - [ ] Bug fix
   - [ ] New feature
   - [ ] Documentation
   - [ ] Refactoring
   - [ ] Performance improvement

   ## Related Issues
   Fixes #123
   Closes #456

   ## Changes Made
   - Added X feature
   - Fixed Y bug
   - Updated Z documentation

   ## Testing Performed
   - [ ] Manual testing
   - [ ] Tested on GCC 11
   - [ ] Tested on Clang 14
   - [ ] Tested on MSVC 19.29
   - [ ] No memory leaks (valgrind/sanitizers)
   - [ ] Documentation updated

   ## Breaking Changes
   - None

   OR

   - Changed API signature of X (see migration guide)

   ## Additional Notes
   Any additional context

Review Process
~~~~~~~~~~~~~~

**What to Expect**:

1. **Automated Checks**: CI/CD runs automatically (if configured)
2. **Code Review**: Maintainers review your code
3. **Feedback**: You may receive change requests
4. **Iteration**: Make requested changes
5. **Approval**: Once approved, your PR will be merged

**Addressing Feedback**:

.. code-block:: bash

   # Make changes based on feedback
   git add .
   git commit -m "refactor: Address review comments"
   git push origin feature/your-feature-name

   # PR automatically updates

**After Merge**:

.. code-block:: bash

   # Update your main branch
   git checkout main
   git pull upstream main

   # Delete feature branch
   git branch -d feature/your-feature-name
   git push origin --delete feature/your-feature-name

Issue Guidelines
----------------

Reporting Bugs
~~~~~~~~~~~~~~

**Before Reporting**:

1. Search existing issues
2. Verify it's a bug (not expected behavior)
3. Test on the latest version
4. Create a minimal reproducible example

**Bug Report Template**:

.. code-block:: markdown

   ## Bug Description
   Clear and concise description of the bug

   ## Steps to Reproduce
   ```cpp
   // Minimal code example
   Database db("test.db");
   auto& table = db.table<Student>("students");
   // ... code that triggers bug
   ```

   ## Expected Behavior
   What should happen

   ## Actual Behavior
   What actually happens

   ## Environment
   - OS: Ubuntu 22.04
   - Compiler: GCC 11.2
   - CMake: 3.24
   - LearnQL version/commit: abc123

   ## Error Messages
   ```
   Full error output
   ```

   ## Additional Context
   Any other relevant information

Feature Requests
~~~~~~~~~~~~~~~~

**Feature Request Template**:

.. code-block:: markdown

   ## Feature Description
   What feature would you like to see?

   ## Use Case
   Why is this feature needed? What problem does it solve?

   ## Proposed Solution
   How should this feature work?

   ```cpp
   // Example usage
   db.beginTransaction();
   table.insert(record);
   db.commit();
   ```

   ## Alternatives Considered
   What other approaches did you consider?

   ## Additional Context
   Any other relevant information

Asking Questions
~~~~~~~~~~~~~~~~

Use the "question" label:

.. code-block:: markdown

   ## Question
   How do I [accomplish task]?

   ## What I've Tried
   - Tried X, but got error Y
   - Read documentation page Z

   ## Environment
   - OS, compiler, etc.

Good First Issues
-----------------

New to Contributing?
~~~~~~~~~~~~~~~~~~~~

Look for issues labeled ``good first issue`` or ``beginner-friendly``.

**Suggested First Contributions**:

1. **Documentation**

   - Fix typos
   - Improve unclear explanations
   - Add code examples
   - Expand tutorials

2. **Examples**

   - Create new example applications
   - Expand existing examples
   - Add comments to examples

3. **Tests**

   - Add unit tests
   - Create integration tests
   - Add performance benchmarks

4. **Code Cleanup**

   - Fix compiler warnings
   - Improve code comments
   - Refactor duplicate code

5. **Small Features**

   - Add utility functions
   - Improve error messages
   - Add convenience methods

Example Good First Issues
~~~~~~~~~~~~~~~~~~~~~~~~~~

- Add missing Doxygen comments to public APIs
- Create a "Hotel Booking System" example
- Write a tutorial on coroutine usage
- Add CMake option to enable sanitizers
- Improve error message when database file is locked
- Add badge to README for C++ standard
- Create benchmark comparing LearnQL to SQLite

Community
---------

Getting Help
~~~~~~~~~~~~

**Documentation**:

- :doc:`../getting-started/quick-start` - Getting started
- :doc:`faq` - Frequently asked questions
- :doc:`cpp20-glossary` - C++20 terminology
- :doc:`examples` - Working examples

**GitHub**:

- Open an issue with "question" label
- Check existing issues and discussions
- Review closed issues for similar problems

**Learning Resources**:

- `learncpp.com <https://www.learncpp.com/>`_
- `cppreference.com <https://en.cppreference.com/>`_
- `C++ Core Guidelines <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines>`_

Staying Updated
~~~~~~~~~~~~~~~

- **Watch** the repository on GitHub for notifications
- **Star** the repository to show support
- Follow the project on social media (if applicable)
- Join the mailing list (if available)

Recognition
~~~~~~~~~~~

Contributors are recognized in:

- ``CONTRIBUTORS.md`` file (if exists)
- Release notes
- Special recognition for significant contributions

License
-------

By contributing, you agree that your contributions will be licensed under the **Academic Free License 3.0** (AFL-3.0).

**What This Means**:

- Your code will be open source
- Others can use your code in academic and commercial projects
- You retain copyright to your contributions
- You grant the project a perpetual license to use your code

**Before Contributing**:

- Ensure you have the right to contribute the code
- Don't include proprietary or copyrighted code
- Don't include code with incompatible licenses

Quick Reference
---------------

Essential Commands
~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Update fork
   git fetch upstream
   git checkout main
   git merge upstream/main

   # Create branch
   git checkout -b feature/my-feature

   # Make changes, then commit
   git add .
   git commit -m "feat: Add my feature"

   # Push and create PR
   git push origin feature/my-feature

   # After PR is merged
   git checkout main
   git pull upstream main
   git branch -d feature/my-feature

Checklist
~~~~~~~~~

Before submitting a PR:

.. code-block:: text

   [ ] Code follows style guidelines
   [ ] Comments added to complex code
   [ ] Doxygen documentation added
   [ ] Code compiles without warnings
   [ ] Tested manually
   [ ] No memory leaks
   [ ] Commit messages follow convention
   [ ] PR description is complete
   [ ] Related issues referenced

Next Steps
----------

**Ready to Contribute?**

1. **Find an Issue**: Browse `issues <https://github.com/yourusername/LearnQL/issues>`_
2. **Or Create One**: Propose your idea first
3. **Fork and Clone**: Set up your environment
4. **Make Changes**: Follow the guidelines
5. **Submit PR**: Open a pull request
6. **Collaborate**: Work with maintainers to refine

**Questions?**

- Check the :doc:`faq`
- Open an issue with the "question" label
- Review existing documentation

**Thank You!**

Your contributions make LearnQL better for everyone. We appreciate your time and effort!

----

**Last Updated**: 2025-11-02

**See Also**:

- :doc:`../getting-started/quick-start` - Get started with LearnQL
- :doc:`faq` - Common questions
- :doc:`examples` - Example code
- :doc:`cpp20-glossary` - C++20 glossary
