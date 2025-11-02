Tutorial 1: Your First Database
================================

In this tutorial, you'll build a complete library management system from scratch using LearnQL's actual API. By the end, you'll understand how to create databases, define models with the property macro system, and perform basic operations.

**Time**: 30 minutes
**Level**: Beginner
**Prerequisites**: C++20 compiler, LearnQL installed

What We'll Build
----------------

A simple library database with:

* Books with titles, authors, ISBN, and publication years
* Basic operations: add books, list books, search books
* Type-safe property system using LEARNQL_PROPERTY macros

Step 1: Project Setup
----------------------

Create a new project directory:

.. code-block:: bash

   mkdir library-db
   cd library-db

Create ``CMakeLists.txt``:

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.23)
   project(LibraryDB CXX)

   set(CMAKE_CXX_STANDARD 20)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)

   # Add LearnQL (adjust path as needed)
   include_directories(/path/to/learnql)

   add_executable(library main.cpp)

Step 2: Define the Book Model
------------------------------

Create ``main.cpp`` and define a Book class using LearnQL's property macros:

.. code-block:: cpp

   #include <learnql/LearnQL.hpp>
   #include <iostream>
   #include <string>

   using namespace learnql;

   class Book {
       LEARNQL_PROPERTIES_BEGIN(Book)
           LEARNQL_PROPERTY(int, book_id, PK)
           LEARNQL_PROPERTY(std::string, title)
           LEARNQL_PROPERTY(std::string, author)
           LEARNQL_PROPERTY(std::string, isbn)
           LEARNQL_PROPERTY(int, publication_year)
       LEARNQL_PROPERTIES_END(
           PROP(int, book_id, PK),
           PROP(std::string, title),
           PROP(std::string, author),
           PROP(std::string, isbn),
           PROP(int, publication_year)
       )

   public:
       Book() = default;

       Book(int id, const std::string& t, const std::string& a,
            const std::string& i, int year)
           : book_id_(id), title_(t), author_(a), isbn_(i),
             publication_year_(year) {}
   };

**Understanding the code:**

* ``LEARNQL_PROPERTIES_BEGIN`` starts the property definitions
* ``LEARNQL_PROPERTY(type, name, PK)`` marks ``book_id`` as the primary key
* Each property automatically generates:

  * Private member with trailing underscore (e.g., ``book_id_``)
  * Public getter: ``get_book_id()``
  * Public setter: ``set_book_id(int value)``
  * Static Field object: ``Book::book_id`` for queries

* ``LEARNQL_PROPERTIES_END`` completes the property system
* The convenience constructor allows easy object creation

Step 3: Create the Database
----------------------------

Add the main function to create the database:

.. code-block:: cpp

   int main() {
       try {
           // Create or open the database file
           core::Database db("library.db");

           // Get a reference to the books table
           auto& books = db.table<Book>("books");

           std::cout << "Library database initialized!" << std::endl;
           std::cout << "Books in catalog: " << books.size() << std::endl;

           return 0;
       } catch (const std::exception& e) {
           std::cerr << "Error: " << e.what() << std::endl;
           return 1;
       }
   }

**Key API points:**

* ``core::Database`` is in the ``core`` namespace
* ``db.table<Book>("books")`` creates or opens a table
* ``books.size()`` returns the number of records

Compile and run:

.. code-block:: bash

   mkdir build && cd build
   cmake ..
   cmake --build .
   ./library

You should see: ``Library database initialized!``

A file named ``library.db`` will be created in the build directory.

Step 4: Add Books
-----------------

Let's add books to the database using the ``insert()`` method:

.. code-block:: cpp

   void add_sample_books(core::Table<Book>& books) {
       std::cout << "Adding sample books..." << std::endl;

       books.insert(Book(1, "The C++ Programming Language",
                        "Bjarne Stroustrup", "978-0321563842", 2013));
       books.insert(Book(2, "Effective Modern C++",
                        "Scott Meyers", "978-1491903995", 2014));
       books.insert(Book(3, "Design Patterns",
                        "Gang of Four", "978-0201633610", 1994));
       books.insert(Book(4, "Clean Code",
                        "Robert C. Martin", "978-0132350884", 2008));
       books.insert(Book(5, "The Pragmatic Programmer",
                        "Hunt & Thomas", "978-0201616224", 1999));

       std::cout << "Added 5 books to the library!" << std::endl;
   }

Update main to call this function:

.. code-block:: cpp

   int main() {
       try {
           core::Database db("library.db");
           auto& books = db.table<Book>("books");

           // Add sample books if table is empty
           if (books.size() == 0) {
               add_sample_books(books);
           }

           std::cout << "Books in catalog: " << books.size() << std::endl;

           return 0;
       } catch (const std::exception& e) {
           std::cerr << "Error: " << e.what() << std::endl;
           return 1;
       }
   }

.. note::
   Each time you run the program, it checks if the table is empty before inserting to avoid duplicate records with the same IDs.

Step 5: List All Books
-----------------------

Create a function to display all books using iteration:

.. code-block:: cpp

   void list_all_books(core::Table<Book>& books) {
       std::cout << "\n=== Library Catalog ===" << std::endl;
       std::cout << std::string(80, '-') << std::endl;

       // Iterate over all books
       for (const auto& book : books) {
           std::cout << "ID: " << book.get_book_id() << std::endl;
           std::cout << "  Title: " << book.get_title() << std::endl;
           std::cout << "  Author: " << book.get_author() << std::endl;
           std::cout << "  ISBN: " << book.get_isbn() << std::endl;
           std::cout << "  Year: " << book.get_publication_year() << std::endl;
           std::cout << std::endl;
       }

       std::cout << "Total books: " << books.size() << std::endl;
   }

**Understanding the API:**

* Table objects support range-based for loops
* Use getter methods to access properties: ``get_title()``, ``get_author()``, etc.
* These getters are automatically generated by the property macros

Update main:

.. code-block:: cpp

   int main() {
       try {
           core::Database db("library.db");
           auto& books = db.table<Book>("books");

           if (books.size() == 0) {
               add_sample_books(books);
           }

           // List all books
           list_all_books(books);

           return 0;
       } catch (const std::exception& e) {
           std::cerr << "Error: " << e.what() << std::endl;
           return 1;
       }
   }

Run the program to see your catalog!

Step 6: Search for Books
-------------------------

Add a search function using the ``where()`` method with static Fields:

.. code-block:: cpp

   void search_by_author(core::Table<Book>& books, const std::string& author_name) {
       std::cout << "\n=== Searching for books by: " << author_name << " ===" << std::endl;

       // Use static Field for type-safe queries
       auto results = books.where(Book::author == author_name);

       if (results.empty()) {
           std::cout << "No books found by this author." << std::endl;
       } else {
           for (const auto& book : results) {
               std::cout << "  - " << book.get_title()
                        << " (" << book.get_publication_year() << ")" << std::endl;
           }
       }
   }

Add another search function for recent books:

.. code-block:: cpp

   void search_recent_books(core::Table<Book>& books, int year_threshold) {
       std::cout << "\n=== Books published after " << year_threshold << " ===" << std::endl;

       // Use static Field with comparison operator
       auto results = books.where(Book::publication_year > year_threshold);

       for (const auto& book : results) {
           std::cout << "  - " << book.get_title()
                    << " (" << book.get_publication_year() << ")" << std::endl;
       }
   }

**Understanding the Query API:**

* ``Book::author`` is a static Field object generated by the property macros
* Use ``==``, ``>``, ``<``, etc. to create query expressions
* ``where()`` returns a range that can be iterated
* No placeholder underscore ``_`` needed - use static Fields directly!

Update main to use these search functions:

.. code-block:: cpp

   int main() {
       try {
           core::Database db("library.db");
           auto& books = db.table<Book>("books");

           if (books.size() == 0) {
               add_sample_books(books);
           }

           list_all_books(books);

           // Search examples
           search_by_author(books, "Scott Meyers");
           search_recent_books(books, 2000);

           return 0;
       } catch (const std::exception& e) {
           std::cerr << "Error: " << e.what() << std::endl;
           return 1;
       }
   }

Step 7: Complete Program
-------------------------

Here's the complete ``main.cpp``:

.. code-block:: cpp

   #include <learnql/LearnQL.hpp>
   #include <iostream>
   #include <string>

   using namespace learnql;

   class Book {
       LEARNQL_PROPERTIES_BEGIN(Book)
           LEARNQL_PROPERTY(int, book_id, PK)
           LEARNQL_PROPERTY(std::string, title)
           LEARNQL_PROPERTY(std::string, author)
           LEARNQL_PROPERTY(std::string, isbn)
           LEARNQL_PROPERTY(int, publication_year)
       LEARNQL_PROPERTIES_END(
           PROP(int, book_id, PK),
           PROP(std::string, title),
           PROP(std::string, author),
           PROP(std::string, isbn),
           PROP(int, publication_year)
       )

   public:
       Book() = default;

       Book(int id, const std::string& t, const std::string& a,
            const std::string& i, int year)
           : book_id_(id), title_(t), author_(a), isbn_(i),
             publication_year_(year) {}
   };

   void add_sample_books(core::Table<Book>& books) {
       std::cout << "Adding sample books..." << std::endl;

       books.insert(Book(1, "The C++ Programming Language",
                        "Bjarne Stroustrup", "978-0321563842", 2013));
       books.insert(Book(2, "Effective Modern C++",
                        "Scott Meyers", "978-1491903995", 2014));
       books.insert(Book(3, "Design Patterns",
                        "Gang of Four", "978-0201633610", 1994));
       books.insert(Book(4, "Clean Code",
                        "Robert C. Martin", "978-0132350884", 2008));
       books.insert(Book(5, "The Pragmatic Programmer",
                        "Hunt & Thomas", "978-0201616224", 1999));

       std::cout << "Added 5 books!" << std::endl;
   }

   void list_all_books(core::Table<Book>& books) {
       std::cout << "\n=== Library Catalog ===" << std::endl;
       std::cout << std::string(80, '-') << std::endl;

       for (const auto& book : books) {
           std::cout << "ID: " << book.get_book_id() << std::endl;
           std::cout << "  Title: " << book.get_title() << std::endl;
           std::cout << "  Author: " << book.get_author() << std::endl;
           std::cout << "  ISBN: " << book.get_isbn() << std::endl;
           std::cout << "  Year: " << book.get_publication_year() << std::endl;
           std::cout << std::endl;
       }

       std::cout << "Total books: " << books.size() << std::endl;
   }

   void search_by_author(core::Table<Book>& books, const std::string& author) {
       std::cout << "\n=== Books by " << author << " ===" << std::endl;

       auto results = books.where(Book::author == author);

       if (results.empty()) {
           std::cout << "No books found." << std::endl;
       } else {
           for (const auto& book : results) {
               std::cout << "  - " << book.get_title()
                        << " (" << book.get_publication_year() << ")" << std::endl;
           }
       }
   }

   void search_recent_books(core::Table<Book>& books, int year) {
       std::cout << "\n=== Books after " << year << " ===" << std::endl;

       auto results = books.where(Book::publication_year > year);

       for (const auto& book : results) {
           std::cout << "  - " << book.get_title()
                    << " (" << book.get_publication_year() << ")" << std::endl;
       }
   }

   int main() {
       try {
           core::Database db("library.db");
           auto& books = db.table<Book>("books");

           if (books.size() == 0) {
               add_sample_books(books);
           }

           list_all_books(books);
           search_by_author(books, "Scott Meyers");
           search_recent_books(books, 2000);

           return 0;
       } catch (const std::exception& e) {
           std::cerr << "Error: " << e.what() << std::endl;
           return 1;
       }
   }

Build and run:

.. code-block:: bash

   cd build
   cmake --build .
   ./library

Expected Output
---------------

.. code-block:: text

   Adding sample books...
   Added 5 books!

   === Library Catalog ===
   --------------------------------------------------------------------------------
   ID: 1
     Title: The C++ Programming Language
     Author: Bjarne Stroustrup
     ISBN: 978-0321563842
     Year: 2013

   ID: 2
     Title: Effective Modern C++
     Author: Scott Meyers
     ISBN: 978-1491903995
     Year: 2014

   ... (more books) ...

   Total books: 5

   === Books by Scott Meyers ===
     - Effective Modern C++ (2014)

   === Books after 2000 ===
     - Effective Modern C++ (2014)
     - The C++ Programming Language (2013)
     - Clean Code (2008)

Exercises
---------

Try these challenges to reinforce your learning:

1. **Add a search by title function**

   .. code-block:: cpp

      void search_by_title(core::Table<Book>& books, const std::string& title);
      // Hint: Use Book::title == title

2. **Find the oldest book in the library**

   .. code-block:: cpp

      // Hint: Iterate and track the minimum publication_year

3. **Count books by decade**

   .. code-block:: cpp

      // Hint: Use where() with year ranges

4. **Add a function to remove a book by ID**

   .. code-block:: cpp

      void remove_book(core::Table<Book>& books, int book_id);
      // Hint: Use books.find(id) and books.remove(id)

What You Learned
----------------

✅ Creating a database with ``core::Database``

✅ Defining models with ``LEARNQL_PROPERTIES_BEGIN/END`` macros

✅ Using ``LEARNQL_PROPERTY`` for automatic getter/setter generation

✅ Marking primary keys with the ``PK`` flag

✅ Creating tables with ``db.table<T>("name")``

✅ Inserting records with ``table.insert()``

✅ Iterating over tables with range-based for loops

✅ Using static Fields for type-safe queries (e.g., ``Book::author``)

✅ Querying with ``where()`` and comparison operators

✅ Accessing properties with auto-generated getters

Next Steps
----------

Continue to :doc:`tutorial-02-crud-operations` to learn about finding, updating, and deleting records using ``find()``, ``update()``, and ``remove()``.

.. tip::
   Try modifying the program to add your own favorite books! Experiment with different queries using the static Field objects to get comfortable with the API.
