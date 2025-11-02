Serialization Module
====================

.. contents:: Table of Contents
   :local:
   :depth: 2

Overview
--------

The Serialization module handles converting C++ objects to/from binary format for storage on disk. LearnQL uses a simple, efficient binary serialization scheme that supports primitive types, strings, and custom types. All serialization happens automatically when you insert or query records.

**Key Features:**

- Automatic serialization for common types (``int``, ``double``, ``std::string``, etc.)
- Type-safe binary encoding/decoding
- Support for custom serialization via specialization
- Efficient fixed-size encoding for primitives
- Variable-length encoding for strings and containers

**Module Components:**

- ``BinaryWriter`` - Writes binary data to a buffer
- ``BinaryReader`` - Reads binary data from a buffer
- ``Serializable`` concept - Type trait for serializable types
- Serialization functions for standard types

.. note::
   You typically don't use this module directly. Serialization happens automatically when using ``Table::insert()``, ``Table::find()``, etc.

Quick Start
-----------

Automatic Serialization
~~~~~~~~~~~~~~~~~~~~~~~

Most types work automatically:

.. code-block:: cpp

   #include <learnql/LearnQL.hpp>

   class Student {
   public:
       LEARNQL_PROPERTIES_BEGIN(Student)
           LEARNQL_PROPERTY(int, id, PK)
           LEARNQL_PROPERTY(std::string, name)
           LEARNQL_PROPERTY(double, gpa)
       LEARNQL_PROPERTIES_END(
           PROP(int, id, PK),
           PROP(std::string, name),
           PROP(double, gpa)
       )
   };

   // Serialization happens automatically
   Database db("school.db");
   auto& students = db.table<Student>("students");

   Student alice{1, "Alice", 3.8};
   students.insert(alice);  // ← Automatically serialized to disk

   auto record = students.find(1);  // ← Automatically deserialized

Custom Serialization
~~~~~~~~~~~~~~~~~~~~

For custom types, provide ``serialize()`` and ``deserialize()`` methods:

.. code-block:: cpp

   struct Color {
       uint8_t r, g, b, a;

       // Custom serialization
       void serialize(serialization::BinaryWriter& writer) const {
           writer.write(r);
           writer.write(g);
           writer.write(b);
           writer.write(a);
       }

       void deserialize(serialization::BinaryReader& reader) {
           r = reader.read<uint8_t>();
           g = reader.read<uint8_t>();
           b = reader.read<uint8_t>();
           a = reader.read<uint8_t>();
       }
   };

   // Now Color can be used in LearnQL types
   class Product {
   public:
       LEARNQL_PROPERTIES_BEGIN(Product)
           LEARNQL_PROPERTY(int, id, PK)
           LEARNQL_PROPERTY(Color, color)  // Custom type!
       LEARNQL_PROPERTIES_END(
           PROP(int, id, PK),
           PROP(Color, color)
       )
   };

Binary Format
-------------

Encoding Scheme
~~~~~~~~~~~~~~~

LearnQL uses little-endian binary encoding:

.. list-table::
   :header-rows: 1
   :widths: 25 25 50

   * - Type
     - Size (bytes)
     - Format
   * - ``int8_t``
     - 1
     - Raw byte
   * - ``int16_t``
     - 2
     - Little-endian 16-bit
   * - ``int32_t``, ``int``
     - 4
     - Little-endian 32-bit
   * - ``int64_t``
     - 8
     - Little-endian 64-bit
   * - ``uint8_t``
     - 1
     - Raw byte
   * - ``uint16_t``
     - 2
     - Little-endian 16-bit
   * - ``uint32_t``
     - 4
     - Little-endian 32-bit
   * - ``uint64_t``
     - 8
     - Little-endian 64-bit
   * - ``float``
     - 4
     - IEEE 754 single precision
   * - ``double``
     - 8
     - IEEE 754 double precision
   * - ``bool``
     - 1
     - 0x00 = false, 0x01 = true
   * - ``std::string``
     - 4 + N
     - 4-byte length + UTF-8 bytes

Example Binary Layout
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   struct Student {
       int id;          // 4 bytes
       std::string name;  // 4 + N bytes
       double gpa;      // 8 bytes
   };

   Student alice{12345, "Alice", 3.8};

Binary representation:

.. code-block:: text

   Offset  Size  Field      Value          Hex
   ──────────────────────────────────────────────────
   0       4     id         12345          39 30 00 00
   4       4     name.len   5              05 00 00 00
   8       5     name.data  "Alice"        41 6C 69 63 65
   13      8     gpa        3.8            66 66 66 66 66 66 0E 40

   Total size: 21 bytes

Class Reference
---------------

BinaryWriter Class
~~~~~~~~~~~~~~~~~~

.. doxygenclass:: learnql::serialization::BinaryWriter
   :members:

Writes binary data to a ``std::vector<uint8_t>`` buffer.

**Constructor:**

.. code-block:: cpp

   BinaryWriter();  // Creates empty writer

Core Methods
^^^^^^^^^^^^

``write()`` - Write Primitive
""""""""""""""""""""""""""""""

.. code-block:: cpp

   template<typename T>
   void write(const T& value);

Writes a primitive value in little-endian format.

**Supported Types:**

- All integer types (``int8_t``, ``int16_t``, ``int32_t``, ``int64_t``, unsigned variants)
- Floating-point types (``float``, ``double``)
- Boolean (``bool``)

**Example:**

.. code-block:: cpp

   BinaryWriter writer;
   writer.write(int32_t{42});
   writer.write(double{3.14159});
   writer.write(true);

``write()`` - Write String
"""""""""""""""""""""""""""

.. code-block:: cpp

   void write(const std::string& str);

Writes a string as 4-byte length followed by UTF-8 bytes.

**Example:**

.. code-block:: cpp

   writer.write(std::string{"Hello, LearnQL!"});
   // Writes: [15, 0, 0, 0, 'H', 'e', 'l', 'l', 'o', ...]

``data()`` - Get Buffer
""""""""""""""""""""""""

.. code-block:: cpp

   [[nodiscard]] const std::vector<uint8_t>& data() const noexcept;

Returns the internal byte buffer.

**Example:**

.. code-block:: cpp

   auto buffer = writer.data();
   std::cout << "Serialized " << buffer.size() << " bytes\n";

``clear()`` - Reset Writer
"""""""""""""""""""""""""""

.. code-block:: cpp

   void clear();

Clears the internal buffer.

BinaryReader Class
~~~~~~~~~~~~~~~~~~

.. doxygenclass:: learnql::serialization::BinaryReader
   :members:

Reads binary data from a ``std::vector<uint8_t>`` buffer.

**Constructor:**

.. code-block:: cpp

   explicit BinaryReader(const std::vector<uint8_t>& buffer);
   explicit BinaryReader(std::vector<uint8_t>&& buffer);

Core Methods
^^^^^^^^^^^^

``read()`` - Read Primitive
""""""""""""""""""""""""""""

.. code-block:: cpp

   template<typename T>
   T read();

Reads and returns a primitive value.

**Supported Types:** Same as ``BinaryWriter::write()``

**Example:**

.. code-block:: cpp

   BinaryReader reader(buffer);
   int32_t id = reader.read<int32_t>();
   double gpa = reader.read<double>();
   bool active = reader.read<bool>();

``read()`` - Read String
"""""""""""""""""""""""""

.. code-block:: cpp

   std::string read<std::string>();

Reads a string (4-byte length + bytes).

**Example:**

.. code-block:: cpp

   std::string name = reader.read<std::string>();

``has_more()`` - Check Remaining Data
""""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   [[nodiscard]] bool has_more() const noexcept;

Returns ``true`` if there's more data to read.

**Example:**

.. code-block:: cpp

   while (reader.has_more()) {
       auto value = reader.read<int>();
       process(value);
   }

``position()`` / ``seek()`` - Buffer Navigation
""""""""""""""""""""""""""""""""""""""""""""""""

.. code-block:: cpp

   [[nodiscard]] std::size_t position() const noexcept;
   void seek(std::size_t pos);

Get or set the current read position.

**Example:**

.. code-block:: cpp

   std::size_t pos = reader.position();  // Remember position
   auto value = reader.read<int>();
   reader.seek(pos);  // Rewind
   auto value2 = reader.read<int>();  // Read again

Serializable Concept
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   template<typename T>
   concept Serializable = requires(T obj, BinaryWriter writer, BinaryReader reader) {
       obj.serialize(writer);
       obj.deserialize(reader);
   };

A type is ``Serializable`` if it has ``serialize()`` and ``deserialize()`` methods.

**Example:**

.. code-block:: cpp

   template<Serializable T>
   void store(const T& obj) {
       BinaryWriter writer;
       obj.serialize(writer);
       // ... write to disk ...
   }

Usage Examples
--------------

Basic Serialization
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <learnql/serialization/BinaryWriter.hpp>
   #include <learnql/serialization/BinaryReader.hpp>

   // Serialize data
   BinaryWriter writer;
   writer.write(int32_t{42});
   writer.write(std::string{"Hello"});
   writer.write(double{3.14});

   // Get serialized bytes
   const auto& buffer = writer.data();
   std::cout << "Serialized " << buffer.size() << " bytes\n";

   // Deserialize
   BinaryReader reader(buffer);
   int32_t num = reader.read<int32_t>();
   std::string str = reader.read<std::string>();
   double pi = reader.read<double>();

   std::cout << "Read: " << num << ", " << str << ", " << pi << "\n";

Custom Type Serialization
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   struct Point {
       double x, y;

       void serialize(BinaryWriter& writer) const {
           writer.write(x);
           writer.write(y);
       }

       void deserialize(BinaryReader& reader) {
           x = reader.read<double>();
           y = reader.read<double>();
       }
   };

   // Use in serialization
   Point p{3.0, 4.0};

   BinaryWriter writer;
   p.serialize(writer);

   BinaryReader reader(writer.data());
   Point p2;
   p2.deserialize(reader);

   std::cout << "Point: (" << p2.x << ", " << p2.y << ")\n";

Nested Structures
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   struct Address {
       std::string street;
       std::string city;
       int zip;

       void serialize(BinaryWriter& writer) const {
           writer.write(street);
           writer.write(city);
           writer.write(zip);
       }

       void deserialize(BinaryReader& reader) {
           street = reader.read<std::string>();
           city = reader.read<std::string>();
           zip = reader.read<int>();
       }
   };

   struct Person {
       std::string name;
       Address address;

       void serialize(BinaryWriter& writer) const {
           writer.write(name);
           address.serialize(writer);  // Nested serialization
       }

       void deserialize(BinaryReader& reader) {
           name = reader.read<std::string>();
           address.deserialize(reader);  // Nested deserialization
       }
   };

Vector/Container Serialization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   struct StudentGrades {
       int student_id;
       std::vector<double> grades;

       void serialize(BinaryWriter& writer) const {
           writer.write(student_id);

           // Write vector size
           writer.write(static_cast<uint32_t>(grades.size()));

           // Write each element
           for (double grade : grades) {
               writer.write(grade);
           }
       }

       void deserialize(BinaryReader& reader) {
           student_id = reader.read<int>();

           // Read vector size
           uint32_t count = reader.read<uint32_t>();

           // Read each element
           grades.clear();
           grades.reserve(count);
           for (uint32_t i = 0; i < count; ++i) {
               grades.push_back(reader.read<double>());
           }
       }
   };

Versioning
~~~~~~~~~~

Support multiple serialization versions:

.. code-block:: cpp

   struct VersionedData {
       static constexpr uint32_t CURRENT_VERSION = 2;

       int id;
       std::string name;
       double value;  // Added in version 2

       void serialize(BinaryWriter& writer) const {
           writer.write(CURRENT_VERSION);
           writer.write(id);
           writer.write(name);
           writer.write(value);
       }

       void deserialize(BinaryReader& reader) {
           uint32_t version = reader.read<uint32_t>();

           id = reader.read<int>();
           name = reader.read<std::string>();

           if (version >= 2) {
               value = reader.read<double>();
           } else {
               value = 0.0;  // Default for v1
           }
       }
   };

Performance Considerations
--------------------------

Serialization Overhead
~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 30 25 45

   * - Type
     - Cost
     - Notes
   * - Primitives (int, double)
     - Negligible
     - Direct memory copy
   * - std::string
     - O(n)
     - Must copy each character
   * - Fixed-size struct
     - Low
     - Sequential writes
   * - Variable-size (vector)
     - O(n)
     - Must write each element

Binary Size
~~~~~~~~~~~

.. code-block:: cpp

   struct CompactData {
       int8_t small_num;     // 1 byte
       uint16_t medium_num;  // 2 bytes
       int32_t large_num;    // 4 bytes
       // Total: 7 bytes
   };

   struct WastedSpace {
       int64_t small_num;    // 8 bytes (overkill for small values)
       int64_t medium_num;   // 8 bytes
       int64_t large_num;    // 8 bytes
       // Total: 24 bytes (3.4x larger!)
   };

**Tip:** Use appropriately sized integer types to minimize serialized size.

String Optimization
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // INEFFICIENT - many small strings
   struct Student {
       std::string first_name;  // 4 + N bytes
       std::string last_name;   // 4 + N bytes
       std::string middle_initial;  // 4 + 1 bytes (wasteful!)
   };

   // BETTER - char for single character
   struct Student {
       std::string first_name;
       std::string last_name;
       char middle_initial;  // 1 byte (no length prefix)

       // Custom serialization for char
       void serialize(BinaryWriter& writer) const {
           writer.write(first_name);
           writer.write(last_name);
           writer.write(static_cast<int8_t>(middle_initial));
       }

       void deserialize(BinaryReader& reader) {
           first_name = reader.read<std::string>();
           last_name = reader.read<std::string>();
           middle_initial = static_cast<char>(reader.read<int8_t>());
       }
   };

Best Practices
--------------

1. **Use Fixed-Size Types for Portability**

   .. code-block:: cpp

      // GOOD - explicit size
      int32_t id;
      int64_t timestamp;

      // AVOID - size varies by platform
      int id;      // 32-bit on some systems, 64-bit on others
      long count;  // 32-bit or 64-bit depending on system

2. **Validate Before Deserialization**

   .. code-block:: cpp

      void deserialize(BinaryReader& reader) {
          uint32_t version = reader.read<uint32_t>();

          if (version > CURRENT_VERSION) {
              throw std::runtime_error("Unknown version: " + std::to_string(version));
          }

          // ... rest of deserialization ...
      }

3. **Reserve Space for Containers**

   .. code-block:: cpp

      void deserialize(BinaryReader& reader) {
          uint32_t count = reader.read<uint32_t>();

          items.clear();
          items.reserve(count);  // ← Avoid reallocations

          for (uint32_t i = 0; i < count; ++i) {
              items.push_back(reader.read<Item>());
          }
      }

4. **Handle Optional Fields Explicitly**

   .. code-block:: cpp

      struct Record {
          int id;
          std::optional<std::string> notes;

          void serialize(BinaryWriter& writer) const {
              writer.write(id);
              writer.write(notes.has_value());
              if (notes) {
                  writer.write(*notes);
              }
          }

          void deserialize(BinaryReader& reader) {
              id = reader.read<int>();
              bool has_notes = reader.read<bool>();
              if (has_notes) {
                  notes = reader.read<std::string>();
              } else {
                  notes = std::nullopt;
              }
          }
      };

5. **Test Round-Trip Serialization**

   .. code-block:: cpp

      void test_serialization() {
          Student original{12345, "Alice", 3.8};

          // Serialize
          BinaryWriter writer;
          original.serialize(writer);

          // Deserialize
          BinaryReader reader(writer.data());
          Student copy;
          copy.deserialize(reader);

          // Verify
          assert(copy.get_id() == original.get_id());
          assert(copy.get_name() == original.get_name());
          assert(copy.get_gpa() == original.get_gpa());
      }

Troubleshooting
---------------

Read Past End of Buffer
~~~~~~~~~~~~~~~~~~~~~~~~

**Problem:** ``std::out_of_range`` exception during deserialization

**Cause:** Reading more data than was written

**Solution:** Ensure deserialize reads same types/order as serialize writes

.. code-block:: cpp

   // WRONG - mismatched read/write
   void serialize(BinaryWriter& w) const {
       w.write(id);
       w.write(name);
   }

   void deserialize(BinaryReader& r) {
       id = r.read<int>();
       name = r.read<std::string>();
       age = r.read<int>();  // ← ERROR: nothing written for age!
   }

Endianness Issues
~~~~~~~~~~~~~~~~~

**Problem:** Data corrupted when moving database between systems

**Cause:** Different byte order (little-endian vs big-endian)

**Solution:** LearnQL always uses little-endian. If you need big-endian support, convert manually:

.. code-block:: cpp

   uint32_t to_little_endian(uint32_t value) {
       if (std::endian::native == std::endian::big) {
           return std::byteswap(value);  // C++23
       }
       return value;
   }

String Encoding Issues
~~~~~~~~~~~~~~~~~~~~~~

**Problem:** Non-ASCII characters appear corrupted

**Cause:** String contains invalid UTF-8

**Solution:** Ensure strings are valid UTF-8 before serialization

.. code-block:: cpp

   bool is_valid_utf8(const std::string& str);  // Implement or use library

   void serialize(BinaryWriter& writer) const {
       if (!is_valid_utf8(name)) {
           throw std::runtime_error("Invalid UTF-8 in name field");
       }
       writer.write(name);
   }

Limitations
-----------

Current Limitations
~~~~~~~~~~~~~~~~~~~

1. **No Built-in Compression**

   Serialized data is not compressed. For large databases, consider external compression.

2. **No Schema Evolution**

   Changing field types or order requires manual versioning (see Versioning example).

3. **No Circular References**

   Cannot serialize circular data structures without custom handling.

4. **No Pointer Serialization**

   Raw pointers cannot be serialized. Use IDs or indices instead.

   .. code-block:: cpp

      // WRONG
      struct Node {
          Node* next;  // Can't serialize pointers!
      };

      // RIGHT
      struct Node {
          std::optional<int> next_id;  // Reference by ID
      };

See Also
--------

- :doc:`storage` - Low-level storage (uses serialization)
- :doc:`reflection` - Type metadata for automatic serialization
- :doc:`../guides/architecture` - System architecture
- :doc:`../tutorials/custom-types` - Tutorial on custom types

**Related Headers:**

- ``learnql/serialization/BinaryWriter.hpp``
- ``learnql/serialization/BinaryReader.hpp``
- ``learnql/serialization/Concepts.hpp``
