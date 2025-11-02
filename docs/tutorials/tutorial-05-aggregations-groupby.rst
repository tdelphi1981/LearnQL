Tutorial 5: Aggregations and GroupBy Operations
===============================================

In this tutorial, you'll learn how to use LearnQL's built-in aggregation functions to analyze data by categories. You'll master ``count_by`` and ``average_by`` operations through the ``query::GroupBy`` class, and build a complete sales analytics system with revenue calculations and business intelligence reports.

**Time**: 25 minutes
**Level**: Intermediate
**Prerequisites**: Completed :doc:`tutorial-04-joins-relationships`

What We'll Build
----------------

A sales analytics system with:

* Sales transactions with products, categories, and amounts
* ``count_by`` to count records per category
* ``average_by`` to calculate averages per group
* Custom aggregations for SUM, MIN, MAX
* Multi-key grouping (category + region)
* Business intelligence reports

Understanding Aggregations
---------------------------

What Are Aggregations?
~~~~~~~~~~~~~~~~~~~~~~

Aggregations summarize multiple records into single values:

* **COUNT**: How many records in each group?
* **AVG**: Average value in each group?
* **SUM**: Total of all values?
* **MIN/MAX**: Smallest/largest value?

SQL Equivalents
~~~~~~~~~~~~~~~

.. code-block:: sql

   -- SQL version
   SELECT department, COUNT(*), AVG(gpa)
   FROM students
   GROUP BY department;

.. code-block:: cpp

   // LearnQL version
   auto dept_counts = query::GroupBy<Student, std::string>::count_by(
       students,
       [](const Student& s) { return s.get_department(); }
   );

   auto dept_avg_gpa = query::GroupBy<Student, std::string>::average_by(
       students,
       [](const Student& s) { return s.get_department(); },
       [](const Student& s) { return s.get_gpa(); }
   );

.. note::
   LearnQL's ``query::GroupBy`` class provides type-safe aggregations with lambda functions for key and value extraction.

Step 1: Define the Sales Model
-------------------------------

Create ``sales_analytics.cpp``:

.. code-block:: cpp

   #include "learnql/LearnQL.hpp"
   #include <iostream>
   #include <string>
   #include <iomanip>

   using namespace learnql;

   struct Sale {
       LEARNQL_PROPERTY(int, sale_id);
       LEARNQL_PROPERTY(std::string, product_name);
       LEARNQL_PROPERTY(std::string, category);
       LEARNQL_PROPERTY(std::string, region);
       LEARNQL_PROPERTY(std::string, salesperson);
       LEARNQL_PROPERTY(double, amount);
       LEARNQL_PROPERTY(int, quantity);
       LEARNQL_PROPERTY(std::string, date);
   };

**Model breakdown:**

* ``category``: Product category (Electronics, Furniture, Books)
* ``region``: Geographic region (North, South, East, West)
* ``salesperson``: Sales representative name
* ``amount``: Sale amount in dollars
* ``quantity``: Number of items sold

Step 2: Adding Sample Data
---------------------------

.. code-block:: cpp

   void populate_sales(Table<Sale>& sales) {
       std::cout << "Populating sales data...\n";

       // Electronics sales
       sales.insert({1, "Laptop", "Electronics", "North", "Alice", 1299.99, 1, "2024-01-15"});
       sales.insert({2, "Mouse", "Electronics", "North", "Alice", 29.99, 5, "2024-01-16"});
       sales.insert({3, "Keyboard", "Electronics", "South", "Bob", 79.99, 3, "2024-01-17"});
       sales.insert({4, "Monitor", "Electronics", "East", "Carol", 299.99, 2, "2024-01-18"});
       sales.insert({5, "Tablet", "Electronics", "North", "Alice", 599.99, 2, "2024-01-19"});

       // Furniture sales
       sales.insert({6, "Desk", "Furniture", "South", "Bob", 399.99, 1, "2024-01-20"});
       sales.insert({7, "Chair", "Furniture", "East", "Carol", 149.99, 4, "2024-01-21"});
       sales.insert({8, "Cabinet", "Furniture", "North", "Alice", 249.99, 2, "2024-01-22"});
       sales.insert({9, "Table", "Furniture", "West", "David", 299.99, 1, "2024-01-23"});

       // Books sales
       sales.insert({10, "C++ Programming", "Books", "North", "Alice", 49.99, 3, "2024-01-24"});
       sales.insert({11, "Database Design", "Books", "South", "Bob", 59.99, 2, "2024-01-25"});
       sales.insert({12, "Algorithms", "Books", "East", "Carol", 54.99, 5, "2024-01-26"});
       sales.insert({13, "Python Basics", "Books", "West", "David", 39.99, 8, "2024-01-27"});

       // More electronics
       sales.insert({14, "Headphones", "Electronics", "South", "Bob", 199.99, 3, "2024-01-28"});
       sales.insert({15, "Webcam", "Electronics", "East", "Carol", 89.99, 2, "2024-01-29"});

       std::cout << "Added " << sales.size() << " sales records\n\n";
   }

Step 3: COUNT BY - Counting Records per Group
----------------------------------------------

The ``count_by`` function counts how many records belong to each category.

Basic Count By Category
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void count_by_category(Table<Sale>& sales) {
       std::cout << "=== Count Sales by Category ===\n";
       std::cout << std::string(60, '-') << "\n";

       // Group by category and count
       auto category_counts = query::GroupBy<Sale, std::string>::count_by(
           sales,
           [](const Sale& s) { return s.get_category(); }
       );

       // Display results
       for (const auto& result : category_counts) {
           std::cout << std::setw(15) << std::left << result.key << " | "
                     << "Sales: " << result.value << "\n";
       }
   }

**How it works:**

1. ``query::GroupBy<Sale, std::string>`` - Template parameters: Record type and Key type
2. ``count_by()`` takes a table and a key extractor lambda
3. Returns ``std::vector<GroupByResult<std::string>>`` with:

   * ``key``: The group key (category name)
   * ``value``: Count of records
   * ``count``: Also contains the count (same as value for count_by)

**Expected output:**

.. code-block:: text

   Electronics     | Sales: 7
   Furniture       | Sales: 4
   Books           | Sales: 4

Count By Salesperson
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void count_by_salesperson(Table<Sale>& sales) {
       std::cout << "\n=== Sales Count by Salesperson ===\n";
       std::cout << std::string(60, '-') << "\n";

       auto salesperson_counts = query::GroupBy<Sale, std::string>::count_by(
           sales,
           [](const Sale& s) { return s.get_salesperson(); }
       );

       // Sort by count descending
       std::sort(salesperson_counts.begin(), salesperson_counts.end(),
           [](const auto& a, const auto& b) {
               return a.value > b.value;
           });

       int rank = 1;
       for (const auto& result : salesperson_counts) {
           std::cout << "#" << rank++ << " "
                     << std::setw(12) << std::left << result.key << " | "
                     << result.value << " sales\n";
       }
   }

Step 4: AVERAGE BY - Calculating Averages per Group
----------------------------------------------------

The ``average_by`` function calculates the average of a numeric field for each group.

Average Sale Amount by Category
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void average_amount_by_category(Table<Sale>& sales) {
       std::cout << "\n=== Average Sale Amount by Category ===\n";
       std::cout << std::string(60, '-') << "\n";

       // Group by category and average the amount
       auto category_avg = query::GroupBy<Sale, std::string>::average_by(
           sales,
           [](const Sale& s) { return s.get_category(); },
           [](const Sale& s) { return s.get_amount(); }
       );

       // Display results
       for (const auto& result : category_avg) {
           std::cout << std::setw(15) << std::left << result.key << " | "
                     << "Avg: $" << std::fixed << std::setprecision(2) << result.value
                     << " (" << result.count << " sales)\n";
       }
   }

**How it works:**

1. First lambda extracts the group key (category)
2. Second lambda extracts the value to average (amount)
3. Returns ``GroupByResult`` with:

   * ``key``: The group key
   * ``value``: Calculated average
   * ``count``: Number of records in the group

**Expected output:**

.. code-block:: text

   Electronics     | Avg: $376.42 (7 sales)
   Furniture       | Avg: $274.99 (4 sales)
   Books           | Avg: $51.24 (4 sales)

Average by Multiple Groups
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Calculate average quantity by region:

.. code-block:: cpp

   void average_quantity_by_region(Table<Sale>& sales) {
       std::cout << "\n=== Average Quantity by Region ===\n";
       std::cout << std::string(60, '-') << "\n";

       auto region_avg_qty = query::GroupBy<Sale, std::string>::average_by(
           sales,
           [](const Sale& s) { return s.get_region(); },
           [](const Sale& s) { return static_cast<double>(s.get_quantity()); }
       );

       for (const auto& result : region_avg_qty) {
           std::cout << std::setw(10) << std::left << result.key << " | "
                     << "Avg Qty: " << std::fixed << std::setprecision(1)
                     << result.value << " items\n";
       }
   }

Step 5: Custom Aggregations - SUM and Total Revenue
----------------------------------------------------

For SUM, MIN, MAX, we implement custom aggregations using manual grouping.

Calculate Total Revenue by Category
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void total_revenue_by_category(Table<Sale>& sales) {
       std::cout << "\n=== Total Revenue by Category ===\n";
       std::cout << std::string(60, '-') << "\n";

       // Manual grouping for SUM
       std::unordered_map<std::string, double> category_revenue;
       std::unordered_map<std::string, int> category_count;

       for (const auto& sale : sales) {
           category_revenue[sale.get_category()] += sale.get_amount();
           category_count[sale.get_category()]++;
       }

       // Display results
       double grand_total = 0.0;
       for (const auto& [category, revenue] : category_revenue) {
           std::cout << std::setw(15) << std::left << category << " | "
                     << "Total: $" << std::fixed << std::setprecision(2) << revenue
                     << " (" << category_count[category] << " sales)\n";
           grand_total += revenue;
       }

       std::cout << std::string(60, '-') << "\n";
       std::cout << std::setw(15) << std::left << "GRAND TOTAL" << " | "
                 << "$" << std::fixed << std::setprecision(2) << grand_total << "\n";
   }

**Why manual grouping?**

LearnQL currently provides ``count_by`` and ``average_by``. For other aggregations like SUM, MIN, MAX, we use standard C++ containers and loops.

Salesperson Performance Report
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Combine COUNT and custom SUM:

.. code-block:: cpp

   void salesperson_performance(Table<Sale>& sales) {
       std::cout << "\n=== Salesperson Performance Report ===\n";
       std::cout << std::string(80, '-') << "\n";

       // Get count by salesperson
       auto counts = query::GroupBy<Sale, std::string>::count_by(
           sales,
           [](const Sale& s) { return s.get_salesperson(); }
       );

       // Calculate revenue by salesperson
       std::unordered_map<std::string, double> revenue;
       for (const auto& sale : sales) {
           revenue[sale.get_salesperson()] += sale.get_amount();
       }

       // Display combined report
       std::cout << std::setw(15) << std::left << "Salesperson"
                 << std::setw(12) << std::right << "Sales"
                 << std::setw(18) << "Total Revenue"
                 << std::setw(15) << "Avg Sale" << "\n";
       std::cout << std::string(80, '-') << "\n";

       for (const auto& result : counts) {
           double total = revenue[result.key];
           double avg = total / result.value;

           std::cout << std::setw(15) << std::left << result.key
                     << std::setw(12) << std::right << result.value
                     << std::setw(18) << std::fixed << std::setprecision(2) << total
                     << std::setw(15) << avg << "\n";
       }
   }

Step 6: Multi-Key Grouping
---------------------------

Group by multiple fields using composite keys.

Revenue by Category and Region
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void revenue_by_category_and_region(Table<Sale>& sales) {
       std::cout << "\n=== Revenue by Category and Region ===\n";
       std::cout << std::string(80, '-') << "\n";

       // Use pair as composite key
       std::map<std::pair<std::string, std::string>, double> revenue;
       std::map<std::pair<std::string, std::string>, int> counts;

       for (const auto& sale : sales) {
           auto key = std::make_pair(sale.get_category(), sale.get_region());
           revenue[key] += sale.get_amount();
           counts[key]++;
       }

       // Display results
       std::cout << std::setw(15) << std::left << "Category"
                 << std::setw(10) << "Region"
                 << std::setw(12) << std::right << "Sales"
                 << std::setw(15) << "Revenue" << "\n";
       std::cout << std::string(80, '-') << "\n";

       for (const auto& [key, rev] : revenue) {
           std::cout << std::setw(15) << std::left << key.first
                     << std::setw(10) << key.second
                     << std::setw(12) << std::right << counts[key]
                     << std::setw(15) << std::fixed << std::setprecision(2)
                     << rev << "\n";
       }
   }

**Expected output:**

.. code-block:: text

   Category        Region         Sales       Revenue
   -------------------------------------------------------------------------------
   Books           East               1        274.95
   Books           North              1        149.97
   Books           South              1        119.98
   Books           West               1        319.92
   Electronics     East               2        389.98
   Electronics     North              3       1929.96
   Electronics     South              2        679.97
   Furniture       East               1        599.96
   Furniture       North              1        499.98
   Furniture       South              1        399.99
   Furniture       West               1        299.99

Step 7: Filtering Groups (HAVING Clause Equivalent)
----------------------------------------------------

Filter results after grouping.

High-Revenue Categories Only
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   void high_revenue_categories(Table<Sale>& sales, double min_revenue) {
       std::cout << "\n=== High-Revenue Categories (> $" << min_revenue << ") ===\n";
       std::cout << std::string(60, '-') << "\n";

       // Calculate revenue by category
       std::unordered_map<std::string, double> category_revenue;
       for (const auto& sale : sales) {
           category_revenue[sale.get_category()] += sale.get_amount();
       }

       // Filter and display
       bool found_any = false;
       for (const auto& [category, revenue] : category_revenue) {
           if (revenue > min_revenue) {
               std::cout << std::setw(15) << std::left << category << " | "
                         << "$" << std::fixed << std::setprecision(2) << revenue << "\n";
               found_any = true;
           }
       }

       if (!found_any) {
           std::cout << "No categories meet the criteria.\n";
       }
   }

Top Performers
~~~~~~~~~~~~~~

Find salespersons with more than a certain number of sales:

.. code-block:: cpp

   void top_performers(Table<Sale>& sales, int min_sales) {
       std::cout << "\n=== Top Performers (" << min_sales << "+ sales) ===\n";
       std::cout << std::string(60, '-') << "\n";

       auto counts = query::GroupBy<Sale, std::string>::count_by(
           sales,
           [](const Sale& s) { return s.get_salesperson(); }
       );

       // Filter by count
       for (const auto& result : counts) {
           if (result.value >= min_sales) {
               std::cout << result.key << ": " << result.value << " sales\n";
           }
       }
   }

Step 8: Complete Sales Analytics Program
-----------------------------------------

.. code-block:: cpp

   int main() {
       try {
           Database db("sales.db");
           auto& sales = db.table<Sale>("sales");

           // Populate data if empty
           if (sales.size() == 0) {
               populate_sales(sales);
           }

           std::cout << "Sales Database: " << sales.size() << " records\n\n";

           // Aggregation demonstrations
           count_by_category(sales);
           count_by_salesperson(sales);

           average_amount_by_category(sales);
           average_quantity_by_region(sales);

           total_revenue_by_category(sales);
           salesperson_performance(sales);

           revenue_by_category_and_region(sales);

           // Filtered aggregations
           high_revenue_categories(sales, 1000.0);
           top_performers(sales, 4);

           return 0;
       } catch (const std::exception& e) {
           std::cerr << "Error: " << e.what() << std::endl;
           return 1;
       }
   }

Aggregation Patterns Summary
-----------------------------

COUNT BY Pattern
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   auto counts = query::GroupBy<RecordType, KeyType>::count_by(
       table,
       [](const RecordType& r) { return r.get_key_field(); }
   );

   for (const auto& result : counts) {
       // result.key   - The group key
       // result.value - Count of records
       // result.count - Same as value
   }

AVERAGE BY Pattern
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   auto averages = query::GroupBy<RecordType, KeyType>::average_by(
       table,
       [](const RecordType& r) { return r.get_key_field(); },
       [](const RecordType& r) { return r.get_value_field(); }
   );

   for (const auto& result : averages) {
       // result.key   - The group key
       // result.value - Calculated average
       // result.count - Number of records in group
   }

Custom Aggregation Pattern (SUM/MIN/MAX)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   std::unordered_map<KeyType, ValueType> aggregated;

   for (const auto& record : table) {
       aggregated[record.get_key()] += record.get_value();  // SUM
       // or: aggregated[key] = std::min(aggregated[key], value); // MIN
       // or: aggregated[key] = std::max(aggregated[key], value); // MAX
   }

Multi-Key Grouping Pattern
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   std::map<std::pair<Key1, Key2>, Value> grouped;

   for (const auto& record : table) {
       auto key = std::make_pair(record.get_key1(), record.get_key2());
       grouped[key] += record.get_value();
   }

What You Learned
----------------

✅ Using ``query::GroupBy::count_by()`` to count records per group

✅ Using ``query::GroupBy::average_by()`` to calculate averages

✅ Understanding ``GroupByResult`` structure (key, value, count)

✅ Implementing custom aggregations (SUM, MIN, MAX) manually

✅ Multi-key grouping with ``std::pair`` as composite keys

✅ Filtering grouped results (HAVING equivalent)

✅ Sorting aggregation results

✅ Building business intelligence reports

✅ Combining multiple aggregations

Exercises
---------

Try these challenges to master aggregations:

1. **Top products report**

   Find the top 5 products by total revenue. Show product name, category, total sales, and total revenue.

2. **Regional performance matrix**

   Create a matrix showing total sales for each category-region combination. Format as a pivot table.

3. **Salesperson rankings**

   Rank salespersons by: (a) total revenue, (b) average sale amount, (c) number of sales. Show all three metrics.

4. **Monthly trend analysis**

   Parse the date field and group by month. Show monthly revenue trends.

5. **Category market share**

   Calculate each category's percentage of total revenue. Display as percentages.

Next Steps
----------

Continue to :doc:`tutorial-06-indexes-performance` to learn about secondary indexes, performance optimization with the fluent API, and fast lookups using ``find_by`` and ``find_all_by``.

.. tip::
   Use LearnQL's built-in ``count_by`` and ``average_by`` for common aggregations. For other operations like SUM, MIN, MAX, use standard C++ containers. This gives you full control and teaches you how aggregations work under the hood!
