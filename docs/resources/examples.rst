Code Examples
=============

This guide provides complete, working examples of real-world applications built with LearnQL. Each example demonstrates practical use cases and can be used as a starting point for your projects.

.. contents:: Quick Navigation
   :local:
   :depth: 2

.. note::
   All examples are self-contained and ready to compile. Copy the code and save it as ``main.cpp``, then build with your preferred method.

Example 1: Simple TODO Application
-----------------------------------

A minimal task management application demonstrating basic CRUD operations.

Description
~~~~~~~~~~~

This TODO app shows:

- Creating a simple database
- Adding, updating, and completing tasks
- Querying pending vs completed tasks
- Basic filtering and display

Complete Code
~~~~~~~~~~~~~

.. code-block:: cpp

   #include <learnql/LearnQL.hpp>
   #include <iostream>
   #include <string>

   using namespace learnql;

   // Task model
   class Task {
       LEARNQL_PROPERTIES_BEGIN(Task)
           LEARNQL_PROPERTY(int, task_id, PK)
           LEARNQL_PROPERTY(std::string, title)
           LEARNQL_PROPERTY(std::string, description)
           LEARNQL_PROPERTY(bool, completed)
           LEARNQL_PROPERTY(int, priority)  // 1=High, 2=Medium, 3=Low
       LEARNQL_PROPERTIES_END(
           PROP(int, task_id, PK),
           PROP(std::string, title),
           PROP(std::string, description),
           PROP(bool, completed),
           PROP(int, priority)
       )

   public:
       Task() = default;
       Task(int id, const std::string& t, const std::string& d, int p)
           : task_id_(id), title_(t), description_(d), completed_(false), priority_(p) {}
   };

   class TodoApp {
       core::Database db_;
       core::Table<Task>& tasks_;
       int next_id_ = 1;

   public:
       TodoApp(const std::string& db_path)
           : db_(db_path), tasks_(db_.table<Task>("tasks")) {
           // Create index on completed status for fast filtering
           tasks_.createIndex<&Task::completed>("idx_completed");
       }

       void addTask(const std::string& title, const std::string& desc, int priority) {
           Task task(next_id_++, title, desc, priority);
           tasks_.insert(task);
           std::cout << "Added task #" << task.task_id() << ": " << title << "\n";
       }

       void completeTask(int task_id) {
           auto task_opt = tasks_.findByKey(task_id);
           if (task_opt) {
               auto task = *task_opt;
               task.set_completed(true);
               tasks_.update(task_id, task);
               std::cout << "Completed task #" << task_id << "\n";
           } else {
               std::cout << "Task #" << task_id << " not found\n";
           }
       }

       void listPending() {
           std::cout << "\n=== Pending Tasks ===\n";
           auto pending = tasks_.query()
               .where(Task::completed == false)
               .orderBy<&Task::priority>(true)  // Sort by priority ascending
               .collect();

           if (pending.empty()) {
               std::cout << "No pending tasks!\n";
               return;
           }

           for (const auto& task : pending) {
               std::string priority_str = task.priority() == 1 ? "[HIGH]" :
                                         task.priority() == 2 ? "[MED]" : "[LOW]";
               std::cout << "#" << task.task_id() << " " << priority_str
                         << " " << task.title() << "\n"
                         << "    " << task.description() << "\n";
           }
       }

       void listCompleted() {
           std::cout << "\n=== Completed Tasks ===\n";
           auto completed = tasks_.query()
               .where(Task::completed == true)
               .collect();

           if (completed.empty()) {
               std::cout << "No completed tasks\n";
               return;
           }

           for (const auto& task : completed) {
               std::cout << "#" << task.task_id() << " " << task.title() << "\n";
           }
       }

       void searchTasks(const std::string& keyword) {
           std::cout << "\n=== Search Results for '" << keyword << "' ===\n";
           auto all_tasks = tasks_.scan();
           bool found = false;

           for (const auto& task : all_tasks) {
               if (task.title().find(keyword) != std::string::npos ||
                   task.description().find(keyword) != std::string::npos) {
                   std::cout << "#" << task.task_id() << " " << task.title() << "\n";
                   found = true;
               }
           }

           if (!found) {
               std::cout << "No tasks found\n";
           }
       }
   };

   int main() {
       TodoApp app("todo.db");

       // Add some tasks
       app.addTask("Buy groceries", "Milk, eggs, bread", 2);
       app.addTask("Finish project", "Complete LearnQL tutorial", 1);
       app.addTask("Call dentist", "Schedule checkup", 3);
       app.addTask("Read book", "Finish C++20 book", 2);

       // List pending tasks
       app.listPending();

       // Complete a task
       app.completeTask(1);

       // Search for tasks
       app.searchTasks("project");

       // Show completed
       app.listCompleted();

       return 0;
   }

Explanation
~~~~~~~~~~~

**Key Features**:

1. **Property Macros**: Task struct uses ``LEARNQL_PROPERTY`` for automatic serialization
2. **Indexing**: Index on ``completed`` field speeds up pending/completed queries
3. **Querying**: Uses WHERE and ORDER BY for filtering and sorting
4. **CRUD Operations**: Create (insert), Read (query), Update, Delete functionality

**How It Works**:

- ``TodoApp`` class wraps database operations
- Tasks are stored persistently in ``todo.db``
- Priority field (1-3) allows task ranking
- Completed flag enables filtering pending vs done tasks

How to Build and Run
~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   # Using g++
   g++ -std=c++20 -I/path/to/LearnQL/include main.cpp -o todo

   # Using CMake
   cmake_minimum_required(VERSION 3.23)
   project(TodoApp)
   set(CMAKE_CXX_STANDARD 20)
   add_subdirectory(path/to/LearnQL)
   add_executable(todo main.cpp)
   target_link_libraries(todo PRIVATE learnql)

   # Run
   ./todo

Possible Extensions
~~~~~~~~~~~~~~~~~~~

1. **Due Dates**: Add ``std::time_t due_date`` field
2. **Categories**: Add ``std::string category`` for grouping tasks
3. **Tags**: Many-to-many relationship with tags
4. **Reminders**: Query tasks due soon
5. **Statistics**: Count tasks by priority/category
6. **CLI Interface**: Parse command-line arguments for interactive use

Example 2: Contact Management System
-------------------------------------

A contact book with phone numbers, addresses, and search functionality.

Description
~~~~~~~~~~~

This example demonstrates:

- Multiple related tables (contacts and phone numbers)
- One-to-many relationships
- Advanced searching and filtering
- Structured data organization

Complete Code
~~~~~~~~~~~~~

.. code-block:: cpp

   #include <learnql/LearnQL.hpp>
   #include <iostream>
   #include <iomanip>

   using namespace learnql;

   // Contact model
   class Contact {
       LEARNQL_PROPERTIES_BEGIN(Contact)
           LEARNQL_PROPERTY(int, contact_id, PK)
           LEARNQL_PROPERTY(std::string, first_name)
           LEARNQL_PROPERTY(std::string, last_name)
           LEARNQL_PROPERTY(std::string, email)
           LEARNQL_PROPERTY(std::string, address)
           LEARNQL_PROPERTY(std::string, city)
       LEARNQL_PROPERTIES_END(
           PROP(int, contact_id, PK),
           PROP(std::string, first_name),
           PROP(std::string, last_name),
           PROP(std::string, email),
           PROP(std::string, address),
           PROP(std::string, city)
       )

   public:
       Contact() = default;
       Contact(int id, const std::string& fname, const std::string& lname,
               const std::string& em, const std::string& addr, const std::string& c)
           : contact_id_(id), first_name_(fname), last_name_(lname),
             email_(em), address_(addr), city_(c) {}

       std::string fullName() const {
           return first_name_ + " " + last_name_;
       }
   };

   // Phone number model (one-to-many with Contact)
   class PhoneNumber {
       LEARNQL_PROPERTIES_BEGIN(PhoneNumber)
           LEARNQL_PROPERTY(int, phone_id, PK)
           LEARNQL_PROPERTY(int, contact_id)  // Foreign key
           LEARNQL_PROPERTY(std::string, phone_type)  // "mobile", "home", "work"
           LEARNQL_PROPERTY(std::string, number)
       LEARNQL_PROPERTIES_END(
           PROP(int, phone_id, PK),
           PROP(int, contact_id),
           PROP(std::string, phone_type),
           PROP(std::string, number)
       )

   public:
       PhoneNumber() = default;
       PhoneNumber(int id, int cid, const std::string& type, const std::string& num)
           : phone_id_(id), contact_id_(cid), phone_type_(type), number_(num) {}
   };

   class ContactBook {
       core::Database db_;
       core::Table<Contact>& contacts_;
       core::Table<PhoneNumber>& phones_;
       int next_contact_id_ = 1;
       int next_phone_id_ = 1;

   public:
       ContactBook(const std::string& db_path)
           : db_(db_path),
             contacts_(db_.table<Contact>("contacts")),
             phones_(db_.table<PhoneNumber>("phone_numbers")) {
           // Create indexes for fast lookups
           contacts_.createIndex<&Contact::last_name>("idx_lastname");
           contacts_.createIndex<&Contact::city>("idx_city");
           phones_.createIndex<&PhoneNumber::contact_id>("idx_contact");
       }

       int addContact(const std::string& fname, const std::string& lname,
                      const std::string& email, const std::string& addr,
                      const std::string& city) {
           int id = next_contact_id_++;
           Contact contact(id, fname, lname, email, addr, city);
           contacts_.insert(contact);
           std::cout << "Added contact: " << contact.fullName() << " (ID: " << id << ")\n";
           return id;
       }

       void addPhoneNumber(int contact_id, const std::string& type,
                          const std::string& number) {
           PhoneNumber phone(next_phone_id_++, contact_id, type, number);
           phones_.insert(phone);
           std::cout << "Added " << type << " phone: " << number << "\n";
       }

       void displayContact(int contact_id) {
           auto contact_opt = contacts_.findByKey(contact_id);
           if (!contact_opt) {
               std::cout << "Contact not found\n";
               return;
           }

           const auto& contact = *contact_opt;
           std::cout << "\n=== Contact Details ===\n";
           std::cout << "Name: " << contact.fullName() << "\n";
           std::cout << "Email: " << contact.email() << "\n";
           std::cout << "Address: " << contact.address() << "\n";
           std::cout << "City: " << contact.city() << "\n";

           // Get phone numbers
           auto numbers = phones_.query()
               .where(PhoneNumber::contact_id == contact_id)
               .collect();

           if (!numbers.empty()) {
               std::cout << "Phone Numbers:\n";
               for (const auto& phone : numbers) {
                   std::cout << "  " << std::setw(10) << std::left << phone.phone_type()
                             << ": " << phone.number() << "\n";
               }
           }
       }

       void searchByName(const std::string& name) {
           std::cout << "\n=== Search Results ===\n";
           auto results = contacts_.scan();
           bool found = false;

           for (const auto& contact : results) {
               std::string full_name = contact.fullName();
               // Case-insensitive search
               auto it = std::search(full_name.begin(), full_name.end(),
                                   name.begin(), name.end(),
                                   [](char a, char b) {
                                       return std::tolower(a) == std::tolower(b);
                                   });

               if (it != full_name.end()) {
                   std::cout << contact.contact_id() << ". " << contact.fullName()
                             << " (" << contact.email() << ")\n";
                   found = true;
               }
           }

           if (!found) {
               std::cout << "No contacts found\n";
           }
       }

       void listByCity(const std::string& city) {
           std::cout << "\n=== Contacts in " << city << " ===\n";
           auto results = contacts_.query()
               .where(Contact::city == city)
               .orderBy<&Contact::last_name>(true)
               .collect();

           if (results.empty()) {
               std::cout << "No contacts in " << city << "\n";
               return;
           }

           for (const auto& contact : results) {
               std::cout << contact.fullName() << " - " << contact.address() << "\n";
           }
       }

       void listAllContacts() {
           std::cout << "\n=== All Contacts ===\n";
           auto all_contacts = contacts_.query()
               .orderBy<&Contact::last_name>(true)
               .collect();

           for (const auto& contact : all_contacts) {
               std::cout << std::setw(3) << contact.contact_id() << ". "
                         << std::setw(25) << std::left << contact.fullName()
                         << " " << contact.email() << "\n";
           }
       }
   };

   int main() {
       ContactBook book("contacts.db");

       // Add contacts
       int id1 = book.addContact("Alice", "Johnson", "alice@example.com",
                                  "123 Main St", "New York");
       book.addPhoneNumber(id1, "mobile", "555-0101");
       book.addPhoneNumber(id1, "home", "555-0102");

       int id2 = book.addContact("Bob", "Smith", "bob@example.com",
                                  "456 Oak Ave", "Boston");
       book.addPhoneNumber(id2, "work", "555-0201");

       int id3 = book.addContact("Charlie", "Brown", "charlie@example.com",
                                  "789 Elm St", "New York");
       book.addPhoneNumber(id3, "mobile", "555-0301");

       // Display specific contact
       book.displayContact(id1);

       // Search by name
       book.searchByName("alice");

       // List by city
       book.listByCity("New York");

       // List all
       book.listAllContacts();

       return 0;
   }

Explanation
~~~~~~~~~~~

**Database Design**:

- **Contacts Table**: Stores contact information
- **Phone Numbers Table**: Stores multiple phones per contact (one-to-many)
- **Indexes**: Speed up searches by last name, city, and contact_id

**Key Techniques**:

1. **Foreign Keys**: ``PhoneNumber.contact_id`` references ``Contact.contact_id``
2. **Joins**: Query phone numbers for a specific contact
3. **Multiple Indexes**: Different indexes for different query patterns
4. **Case-Insensitive Search**: Custom search logic

How to Build and Run
~~~~~~~~~~~~~~~~~~~~

Same as Example 1. Save as ``main.cpp`` and compile with C++20.

Possible Extensions
~~~~~~~~~~~~~~~~~~~

1. **Email Addresses**: Add separate email table for multiple emails
2. **Groups/Tags**: Categorize contacts (family, work, friends)
3. **Import/Export**: VCard or CSV support
4. **Birthdays**: Add birthday field and query upcoming birthdays
5. **Photo**: Store contact photo path
6. **Social Media**: Add social media handles

Example 3: Blog System with Posts and Comments
-----------------------------------------------

A simple blogging platform demonstrating hierarchical data and relationships.

Description
~~~~~~~~~~~

Features:

- Blog posts with authors
- Comments on posts
- Nested relationships
- Aggregations (comment count)
- Time-based queries

Complete Code
~~~~~~~~~~~~~

.. code-block:: cpp

   #include <learnql/LearnQL.hpp>
   #include <iostream>
   #include <ctime>
   #include <iomanip>

   using namespace learnql;

   // Blog post model
   class Post {
       LEARNQL_PROPERTIES_BEGIN(Post)
           LEARNQL_PROPERTY(int, post_id, PK)
           LEARNQL_PROPERTY(std::string, title)
           LEARNQL_PROPERTY(std::string, content)
           LEARNQL_PROPERTY(std::string, author)
           LEARNQL_PROPERTY(int, created_at)  // Unix timestamp
           LEARNQL_PROPERTY(int, view_count)
       LEARNQL_PROPERTIES_END(
           PROP(int, post_id, PK),
           PROP(std::string, title),
           PROP(std::string, content),
           PROP(std::string, author),
           PROP(int, created_at),
           PROP(int, view_count)
       )

   public:
       Post() = default;
       Post(int id, const std::string& t, const std::string& c, const std::string& a)
           : post_id_(id), title_(t), content_(c), author_(a),
             created_at_(std::time(nullptr)), view_count_(0) {}
   };

   // Comment model
   class Comment {
       LEARNQL_PROPERTIES_BEGIN(Comment)
           LEARNQL_PROPERTY(int, comment_id, PK)
           LEARNQL_PROPERTY(int, post_id)  // Foreign key
           LEARNQL_PROPERTY(std::string, author)
           LEARNQL_PROPERTY(std::string, content)
           LEARNQL_PROPERTY(int, created_at)
       LEARNQL_PROPERTIES_END(
           PROP(int, comment_id, PK),
           PROP(int, post_id),
           PROP(std::string, author),
           PROP(std::string, content),
           PROP(int, created_at)
       )

   public:
       Comment() = default;
       Comment(int id, int pid, const std::string& a, const std::string& c)
           : comment_id_(id), post_id_(pid), author_(a), content_(c),
             created_at_(std::time(nullptr)) {}
   };

   class BlogSystem {
       core::Database db_;
       core::Table<Post>& posts_;
       core::Table<Comment>& comments_;
       int next_post_id_ = 1;
       int next_comment_id_ = 1;

   public:
       BlogSystem(const std::string& db_path)
           : db_(db_path),
             posts_(db_.table<Post>("posts")),
             comments_(db_.table<Comment>("comments")) {
           // Create indexes
           posts_.createIndex<&Post::author>("idx_author");
           comments_.createIndex<&Comment::post_id>("idx_post");
       }

       int createPost(const std::string& title, const std::string& content,
                      const std::string& author) {
           int id = next_post_id_++;
           Post post(id, title, content, author);
           posts_.insert(post);
           std::cout << "Created post #" << id << ": " << title << "\n";
           return id;
       }

       void addComment(int post_id, const std::string& author,
                      const std::string& content) {
           Comment comment(next_comment_id_++, post_id, author, content);
           comments_.insert(comment);
           std::cout << "Added comment by " << author << "\n";
       }

       void viewPost(int post_id) {
           auto post_opt = posts_.findByKey(post_id);
           if (!post_opt) {
               std::cout << "Post not found\n";
               return;
           }

           auto post = *post_opt;

           // Increment view count
           post.set_view_count(post.view_count() + 1);
           posts_.update(post_id, post);

           // Display post
           std::cout << "\n" << std::string(60, '=') << "\n";
           std::cout << post.title() << "\n";
           std::cout << "By " << post.author() << " | ";
           std::cout << "Views: " << post.view_count() << "\n";
           std::cout << std::string(60, '-') << "\n";
           std::cout << post.content() << "\n";
           std::cout << std::string(60, '=') << "\n";

           // Display comments
           auto post_comments = comments_.query()
               .where(Comment::post_id == post_id)
               .collect();

           std::cout << "\n--- Comments (" << post_comments.size() << ") ---\n";
           for (const auto& comment : post_comments) {
               std::cout << "\n" << comment.author() << " wrote:\n";
               std::cout << comment.content() << "\n";
           }
       }

       void listPosts() {
           std::cout << "\n=== All Blog Posts ===\n";
           auto all_posts = posts_.scan();

           for (const auto& post : all_posts) {
               // Count comments for this post
               int comment_count = 0;
               for (const auto& comment : comments_.scan()) {
                   if (comment.post_id() == post.post_id()) {
                       ++comment_count;
                   }
               }

               std::cout << "#" << post.post_id() << " "
                         << post.title() << "\n"
                         << "    By " << post.author()
                         << " | " << comment_count << " comments"
                         << " | " << post.view_count() << " views\n";
           }
       }

       void postsByAuthor(const std::string& author) {
           std::cout << "\n=== Posts by " << author << " ===\n";
           auto author_posts = posts_.query()
               .where(Post::author == author)
               .collect();

           if (author_posts.empty()) {
               std::cout << "No posts by " << author << "\n";
               return;
           }

           for (const auto& post : author_posts) {
               std::cout << post.title() << " (" << post.view_count() << " views)\n";
           }
       }

       void popularPosts(int min_views) {
           std::cout << "\n=== Popular Posts (>" << min_views << " views) ===\n";
           auto popular = posts_.query()
               .where(Post::view_count > min_views)
               .orderBy<&Post::view_count>(false)  // Descending
               .collect();

           for (const auto& post : popular) {
               std::cout << post.title() << " - " << post.view_count() << " views\n";
           }
       }
   };

   int main() {
       BlogSystem blog("blog.db");

       // Create posts
       int post1 = blog.createPost(
           "Getting Started with C++20",
           "C++20 brings many new features including concepts, ranges, and coroutines...",
           "Alice"
       );

       int post2 = blog.createPost(
           "Understanding Expression Templates",
           "Expression templates are a powerful technique for building DSLs in C++...",
           "Bob"
       );

       int post3 = blog.createPost(
           "Building a Database in C++",
           "Learn how to implement a simple database engine from scratch...",
           "Alice"
       );

       // Add comments
       blog.addComment(post1, "Charlie", "Great introduction!");
       blog.addComment(post1, "David", "Very helpful, thanks!");
       blog.addComment(post2, "Eve", "Excellent explanation of a complex topic.");

       // View a post (increments view count)
       blog.viewPost(post1);
       blog.viewPost(post1);  // View again
       blog.viewPost(post2);

       // List all posts
       blog.listPosts();

       // Posts by author
       blog.postsByAuthor("Alice");

       // Popular posts
       blog.popularPosts(1);

       return 0;
   }

Explanation
~~~~~~~~~~~

**Features Demonstrated**:

1. **One-to-Many**: Posts have many comments
2. **Aggregation**: Count comments per post
3. **Updates**: Increment view count
4. **Filtering**: Posts by author, popular posts
5. **Sorting**: Order by views (descending)

How to Build and Run
~~~~~~~~~~~~~~~~~~~~

Same as previous examples.

Possible Extensions
~~~~~~~~~~~~~~~~~~~

1. **Categories**: Add category field and filtering
2. **Tags**: Many-to-many relationship with tags
3. **Drafts**: Add published/draft status
4. **Likes**: Track post/comment likes
5. **Edit History**: Store post revisions
6. **Search**: Full-text search in content

Example 4: E-Commerce Product Catalog
--------------------------------------

Product catalog with categories, inventory, and pricing.

Description
~~~~~~~~~~~

Demonstrates:

- Product hierarchy (categories)
- Stock management
- Price queries and filtering
- Product search

Complete Code
~~~~~~~~~~~~~

.. code-block:: cpp

   #include <learnql/LearnQL.hpp>
   #include <iostream>
   #include <iomanip>

   using namespace learnql;

   class Product {
       LEARNQL_PROPERTIES_BEGIN(Product)
           LEARNQL_PROPERTY(int, product_id, PK)
           LEARNQL_PROPERTY(std::string, name)
           LEARNQL_PROPERTY(std::string, category)
           LEARNQL_PROPERTY(double, price)
           LEARNQL_PROPERTY(int, stock_quantity)
           LEARNQL_PROPERTY(std::string, description)
       LEARNQL_PROPERTIES_END(
           PROP(int, product_id, PK),
           PROP(std::string, name),
           PROP(std::string, category),
           PROP(double, price),
           PROP(int, stock_quantity),
           PROP(std::string, description)
       )

   public:
       Product() = default;
       Product(int id, const std::string& n, const std::string& cat,
               double p, int stock, const std::string& desc)
           : product_id_(id), name_(n), category_(cat), price_(p),
             stock_quantity_(stock), description_(desc) {}

       bool inStock() const { return stock_quantity_ > 0; }
   };

   class ProductCatalog {
       core::Database db_;
       core::Table<Product>& products_;
       int next_id_ = 1;

   public:
       ProductCatalog(const std::string& db_path)
           : db_(db_path), products_(db_.table<Product>("products")) {
           products_.createIndex<&Product::category>("idx_category");
           products_.createIndex<&Product::price>("idx_price");
       }

       void addProduct(const std::string& name, const std::string& category,
                      double price, int stock, const std::string& desc) {
           Product product(next_id_++, name, category, price, stock, desc);
           products_.insert(product);
           std::cout << "Added product: " << name << " ($" << price << ")\n";
       }

       void updateStock(int product_id, int quantity) {
           auto product_opt = products_.findByKey(product_id);
           if (product_opt) {
               auto product = *product_opt;
               product.set_stock_quantity(product.stock_quantity() + quantity);
               products_.update(product_id, product);
               std::cout << "Updated stock for " << product.name()
                         << " to " << product.stock_quantity() << "\n";
           }
       }

       void displayProduct(int product_id) {
           auto product_opt = products_.findByKey(product_id);
           if (!product_opt) {
               std::cout << "Product not found\n";
               return;
           }

           const auto& p = *product_opt;
           std::cout << "\n=== Product Details ===\n";
           std::cout << "ID: " << p.product_id() << "\n";
           std::cout << "Name: " << p.name() << "\n";
           std::cout << "Category: " << p.category() << "\n";
           std::cout << "Price: $" << std::fixed << std::setprecision(2) << p.price() << "\n";
           std::cout << "Stock: " << p.stock_quantity()
                     << (p.inStock() ? " (In Stock)" : " (Out of Stock)") << "\n";
           std::cout << "Description: " << p.description() << "\n";
       }

       void listByCategory(const std::string& category) {
           std::cout << "\n=== " << category << " ===\n";
           auto items = products_.query()
               .where(Product::category == category)
               .orderBy<&Product::name>(true)
               .collect();

           for (const auto& p : items) {
               std::cout << std::setw(30) << std::left << p.name()
                         << " $" << std::fixed << std::setprecision(2) << std::setw(8) << p.price()
                         << " (" << p.stock_quantity() << " in stock)\n";
           }
       }

       void findInPriceRange(double min_price, double max_price) {
           std::cout << "\n=== Products $" << min_price << " - $" << max_price << " ===\n";
           auto results = products_.query()
               .where(Product::price >= min_price && Product::price <= max_price)
               .orderBy<&Product::price>(true)
               .collect();

           for (const auto& p : results) {
               std::cout << p.name() << " - $" << p.price()
                         << " (" << p.category() << ")\n";
           }
       }

       void lowStockAlert(int threshold) {
           std::cout << "\n=== Low Stock Alert (<= " << threshold << ") ===\n";
           auto low_stock = products_.query()
               .where(Product::stock_quantity <= threshold)
               .collect();

           if (low_stock.empty()) {
               std::cout << "All products have sufficient stock\n";
               return;
           }

           for (const auto& p : low_stock) {
               std::cout << "⚠ " << p.name() << ": " << p.stock_quantity() << " remaining\n";
           }
       }

       void searchProducts(const std::string& keyword) {
           std::cout << "\n=== Search: '" << keyword << "' ===\n";
           bool found = false;

           for (const auto& p : products_.scan()) {
               if (p.name().find(keyword) != std::string::npos ||
                   p.description().find(keyword) != std::string::npos) {
                   std::cout << p.name() << " - $" << p.price() << "\n";
                   found = true;
               }
           }

           if (!found) {
               std::cout << "No products found\n";
           }
       }
   };

   int main() {
       ProductCatalog catalog("products.db");

       // Add products
       catalog.addProduct("Laptop", "Electronics", 999.99, 15,
                         "High-performance laptop with 16GB RAM");
       catalog.addProduct("Mouse", "Electronics", 29.99, 50,
                         "Wireless optical mouse");
       catalog.addProduct("Desk Chair", "Furniture", 199.99, 5,
                         "Ergonomic office chair");
       catalog.addProduct("Coffee Maker", "Appliances", 79.99, 20,
                         "12-cup programmable coffee maker");
       catalog.addProduct("Bookshelf", "Furniture", 149.99, 8,
                         "5-shelf wooden bookshelf");

       // Display specific product
       catalog.displayProduct(1);

       // List by category
       catalog.listByCategory("Electronics");

       // Find in price range
       catalog.findInPriceRange(50.0, 200.0);

       // Low stock alert
       catalog.lowStockAlert(10);

       // Search
       catalog.searchProducts("chair");

       // Update stock
       catalog.updateStock(1, -5);  // Sell 5 laptops
       catalog.lowStockAlert(10);

       return 0;
   }

Explanation
~~~~~~~~~~~

**Business Logic**:

- **Inventory Management**: Track stock levels
- **Price Filtering**: Find products in budget
- **Category Browsing**: Organize by product type
- **Stock Alerts**: Notify when stock is low

Possible Extensions
~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. **Reviews**: Add review/rating system
2. **Orders**: Track customer orders
3. **Discounts**: Implement sale prices
4. **Suppliers**: Track product suppliers
5. **Images**: Store product image paths
6. **Variants**: Product variations (size, color)

Example 5: Student Grade Tracker
---------------------------------

Academic grade management system.

Description
~~~~~~~~~~~

Track student performance across courses and calculate statistics.

Complete Code
~~~~~~~~~~~~~

.. code-block:: cpp

   #include <learnql/LearnQL.hpp>
   #include <iostream>
   #include <numeric>
   #include <iomanip>

   using namespace learnql;

   class Student {
       LEARNQL_PROPERTIES_BEGIN(Student)
           LEARNQL_PROPERTY(int, student_id, PK)
           LEARNQL_PROPERTY(std::string, name)
           LEARNQL_PROPERTY(std::string, major)
           LEARNQL_PROPERTY(int, year)  // 1-4
       LEARNQL_PROPERTIES_END(
           PROP(int, student_id, PK),
           PROP(std::string, name),
           PROP(std::string, major),
           PROP(int, year)
       )

   public:
       Student() = default;
       Student(int id, const std::string& n, const std::string& m, int y)
           : student_id_(id), name_(n), major_(m), year_(y) {}
   };

   class Grade {
       LEARNQL_PROPERTIES_BEGIN(Grade)
           LEARNQL_PROPERTY(int, grade_id, PK)
           LEARNQL_PROPERTY(int, student_id)
           LEARNQL_PROPERTY(std::string, course_code)
           LEARNQL_PROPERTY(double, score)  // 0-100
           LEARNQL_PROPERTY(int, credits)
       LEARNQL_PROPERTIES_END(
           PROP(int, grade_id, PK),
           PROP(int, student_id),
           PROP(std::string, course_code),
           PROP(double, score),
           PROP(int, credits)
       )

   public:
       Grade() = default;
       Grade(int id, int sid, const std::string& course, double s, int cr)
           : grade_id_(id), student_id_(sid), course_code_(course),
             score_(s), credits_(cr) {}

       char letterGrade() const {
           if (score_ >= 90) return 'A';
           if (score_ >= 80) return 'B';
           if (score_ >= 70) return 'C';
           if (score_ >= 60) return 'D';
           return 'F';
       }

       double gradePoints() const {
           if (score_ >= 90) return 4.0;
           if (score_ >= 80) return 3.0;
           if (score_ >= 70) return 2.0;
           if (score_ >= 60) return 1.0;
           return 0.0;
       }
   };

   class GradeTracker {
       core::Database db_;
       core::Table<Student>& students_;
       core::Table<Grade>& grades_;
       int next_student_id_ = 1;
       int next_grade_id_ = 1;

   public:
       GradeTracker(const std::string& db_path)
           : db_(db_path),
             students_(db_.table<Student>("students")),
             grades_(db_.table<Grade>("grades")) {
           grades_.createIndex<&Grade::student_id>("idx_student");
       }

       int addStudent(const std::string& name, const std::string& major, int year) {
           int id = next_student_id_++;
           Student student(id, name, major, year);
           students_.insert(student);
           std::cout << "Added student: " << name << " (ID: " << id << ")\n";
           return id;
       }

       void addGrade(int student_id, const std::string& course,
                    double score, int credits) {
           Grade grade(next_grade_id_++, student_id, course, score, credits);
           grades_.insert(grade);
           std::cout << "Added grade for " << course << ": "
                     << score << " (" << grade.letterGrade() << ")\n";
       }

       void studentTranscript(int student_id) {
           auto student_opt = students_.findByKey(student_id);
           if (!student_opt) {
               std::cout << "Student not found\n";
               return;
           }

           const auto& student = *student_opt;
           std::cout << "\n=== Transcript ===\n";
           std::cout << "Student: " << student.name() << "\n";
           std::cout << "Major: " << student.major() << " (Year " << student.year() << ")\n\n";

           auto student_grades = grades_.query()
               .where(Grade::student_id == student_id)
               .collect();

           if (student_grades.empty()) {
               std::cout << "No grades recorded\n";
               return;
           }

           std::cout << std::setw(15) << std::left << "Course"
                     << std::setw(10) << "Score"
                     << std::setw(10) << "Grade"
                     << std::setw(10) << "Credits" << "\n";
           std::cout << std::string(45, '-') << "\n";

           double total_points = 0.0;
           int total_credits = 0;

           for (const auto& grade : student_grades) {
               std::cout << std::setw(15) << grade.course_code()
                         << std::setw(10) << grade.score()
                         << std::setw(10) << grade.letterGrade()
                         << std::setw(10) << grade.credits() << "\n";

               total_points += grade.gradePoints() * grade.credits();
               total_credits += grade.credits();
           }

           double gpa = total_credits > 0 ? total_points / total_credits : 0.0;
           std::cout << std::string(45, '-') << "\n";
           std::cout << "GPA: " << std::fixed << std::setprecision(2) << gpa << "\n";
       }

       void courseStatistics(const std::string& course_code) {
           std::cout << "\n=== Statistics for " << course_code << " ===\n";

           auto course_grades = grades_.query()
               .where(Grade::course_code == course_code)
               .collect();

           if (course_grades.empty()) {
               std::cout << "No grades for this course\n";
               return;
           }

           std::vector<double> scores;
           for (const auto& grade : course_grades) {
               scores.push_back(grade.score());
           }

           double avg = std::accumulate(scores.begin(), scores.end(), 0.0) / scores.size();
           double min = *std::min_element(scores.begin(), scores.end());
           double max = *std::max_element(scores.begin(), scores.end());

           std::cout << "Students enrolled: " << scores.size() << "\n";
           std::cout << "Average score: " << std::fixed << std::setprecision(1) << avg << "\n";
           std::cout << "Highest score: " << max << "\n";
           std::cout << "Lowest score: " << min << "\n";

           // Grade distribution
           int a_count = 0, b_count = 0, c_count = 0, d_count = 0, f_count = 0;
           for (const auto& grade : course_grades) {
               char letter = grade.letterGrade();
               if (letter == 'A') ++a_count;
               else if (letter == 'B') ++b_count;
               else if (letter == 'C') ++c_count;
               else if (letter == 'D') ++d_count;
               else ++f_count;
           }

           std::cout << "\nGrade Distribution:\n";
           std::cout << "A: " << a_count << " | B: " << b_count << " | C: " << c_count
                     << " | D: " << d_count << " | F: " << f_count << "\n";
       }

       void honorRoll(double min_gpa) {
           std::cout << "\n=== Honor Roll (GPA >= " << min_gpa << ") ===\n";

           for (const auto& student : students_.scan()) {
               auto student_grades = grades_.query()
                   .where(Grade::student_id == student.student_id())
                   .collect();

               if (student_grades.empty()) continue;

               double total_points = 0.0;
               int total_credits = 0;

               for (const auto& grade : student_grades) {
                   total_points += grade.gradePoints() * grade.credits();
                   total_credits += grade.credits();
               }

               double gpa = total_credits > 0 ? total_points / total_credits : 0.0;

               if (gpa >= min_gpa) {
                   std::cout << student.name() << " (" << student.major() << ") - GPA: "
                             << std::fixed << std::setprecision(2) << gpa << "\n";
               }
           }
       }
   };

   int main() {
       GradeTracker tracker("grades.db");

       // Add students
       int alice = tracker.addStudent("Alice Johnson", "Computer Science", 2);
       int bob = tracker.addStudent("Bob Smith", "Mathematics", 3);
       int charlie = tracker.addStudent("Charlie Brown", "Physics", 2);

       // Add grades for Alice
       tracker.addGrade(alice, "CS101", 95.0, 4);
       tracker.addGrade(alice, "MATH201", 88.0, 3);
       tracker.addGrade(alice, "PHYS101", 92.0, 4);

       // Add grades for Bob
       tracker.addGrade(bob, "MATH201", 78.0, 3);
       tracker.addGrade(bob, "MATH301", 85.0, 4);

       // Add grades for Charlie
       tracker.addGrade(charlie, "PHYS101", 91.0, 4);
       tracker.addGrade(charlie, "CS101", 87.0, 4);

       // View transcript
       tracker.studentTranscript(alice);

       // Course statistics
       tracker.courseStatistics("CS101");
       tracker.courseStatistics("MATH201");

       // Honor roll
       tracker.honorRoll(3.5);

       return 0;
   }

Possible Extensions
~~~~~~~~~~~~~~~~~~~

1. **Attendance**: Track class attendance
2. **Assignments**: Individual assignment scores
3. **Semesters**: Track grades by semester
4. **Prerequisites**: Course prerequisite tracking
5. **Advisors**: Assign academic advisors
6. **Degree Progress**: Track major requirements

Additional Examples
-------------------

For brevity, here are outlines for examples 6-10:

Example 6: Movie Database with Ratings
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Models**: Movie, Rating, Actor, Genre

**Features**:
- Many-to-many relationships (movies-actors, movies-genres)
- Average ratings calculation
- Search by genre, actor, rating
- Top-rated movies query

**Key Queries**:

.. code-block:: cpp

   auto top_movies = movies.query()
       .where(Movie::avg_rating > 4.0)
       .orderBy<&Movie::avg_rating>(false)
       .collect();

Example 7: Inventory Management System
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Models**: Item, Transaction, Location, Supplier

**Features**:
- Stock tracking across locations
- Transaction history
- Reorder alerts
- Supplier management

**Key Queries**:

.. code-block:: cpp

   auto low_stock = items.query()
       .where(Item::quantity < Item::reorder_point)
       .collect();

Example 8: Event Scheduling System
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Models**: Event, Participant, Room, Registration

**Features**:
- Event calendar
- Room booking
- Attendance tracking
- Conflict detection

**Key Queries**:

.. code-block:: cpp

   auto upcoming = events.query()
       .where(Event::date >= today)
       .orderBy<&Event::date>(true)
       .collect();

Example 9: Recipe Database
~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Models**: Recipe, Ingredient, RecipeIngredient, Category

**Features**:
- Recipe search
- Ingredient filtering
- Nutritional information
- Cooking time queries

**Key Queries**:

.. code-block:: cpp

   auto quick_recipes = recipes.query()
       .where(Recipe::prep_time <= 30)
       .collect();

Example 10: Financial Transaction Log
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Models**: Account, Transaction, Category

**Features**:
- Income/expense tracking
- Balance calculation
- Category summaries
- Date-range queries

**Key Queries**:

.. code-block:: cpp

   auto monthly_expenses = transactions.query()
       .where(Transaction::type == "expense" &&
              Transaction::date >= month_start &&
              Transaction::date <= month_end)
       .groupBy<&Transaction::category>()
       .aggregate(sumAmount)
       .collect();

Building and Running Examples
------------------------------

CMakeLists.txt Template
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cmake

   cmake_minimum_required(VERSION 3.23)
   project(LearnQLExamples)

   set(CMAKE_CXX_STANDARD 20)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)

   # Add LearnQL
   add_subdirectory(path/to/LearnQL)

   # Add each example
   add_executable(todo example1_todo.cpp)
   target_link_libraries(todo PRIVATE learnql)

   add_executable(contacts example2_contacts.cpp)
   target_link_libraries(contacts PRIVATE learnql)

   # ... add more examples

Compilation
~~~~~~~~~~~

.. code-block:: bash

   # Create build directory
   mkdir build && cd build

   # Configure
   cmake ..

   # Build all examples
   cmake --build .

   # Or build specific example
   cmake --build . --target todo

   # Run
   ./todo

Next Steps
----------

**Learning Path**:

1. Start with Example 1 (TODO app) - simplest
2. Try Example 2 (Contacts) - adds relationships
3. Explore Example 3 (Blog) - complex queries
4. Build your own application!

**Resources**:

- :doc:`../tutorials/tutorial-01-first-database` - Step-by-step tutorial
- :doc:`../api/query` - Query DSL reference
- :doc:`cpp20-glossary` - C++20 concepts
- :doc:`faq` - Common questions

**Practice Ideas**:

1. Modify examples to add features
2. Combine concepts from multiple examples
3. Build a unique application
4. Share your creation!

**See Also**: :doc:`contributing` to contribute your own examples!

----

**Last Updated**: 2025-11-02

**Source Code**: All examples are available in the ``docs/resources/examples/`` directory (if applicable).
