Property Macros
===============

The Property Macro System is one of LearnQL's most powerful features, enabling automatic code generation that reduces boilerplate and ensures type safety. This guide explores how to use property macros effectively.

Why Property Macros?
--------------------

Traditional C++ Approach
~~~~~~~~~~~~~~~~~~~~~~~~

Without LearnQL, database-aware classes require significant boilerplate:

.. code-block:: cpp

   struct Student {
       int id_;
       std::string name_;
       int age_;

       // Manual getters
       int get_id() const { return id_; }
       const std::string& get_name() const { return name_; }
       int get_age() const { return age_; }

       // Manual setters
       void set_id(int value) { id_ = value; }
       void set_name(const std::string& value) { name_ = value; }
       void set_age(int value) { age_ = value; }

       // Manual serialization
       void serialize(std::ostream& out) const {
           out.write(reinterpret_cast<const char*>(&id_), sizeof(id_));
           size_t name_len = name_.size();
           out.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
           out.write(name_.data(), name_len);
           out.write(reinterpret_cast<const char*>(&age_), sizeof(age_));
       }

       // Manual deserialization
       void deserialize(std::istream& in) {
           in.read(reinterpret_cast<char*>(&id_), sizeof(id_));
           size_t name_len;
           in.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));
           name_.resize(name_len);
           in.read(name_.data(), name_len);
           in.read(reinterpret_cast<char*>(&age_), sizeof(age_));
       }

       // Manual reflection
       static constexpr size_t property_count() { return 3; }
       // ... and more boilerplate
   };

**This is ~70 lines of repetitive code for just 3 fields!**

With LearnQL Property Macros
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

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

       // Optional: convenience constructor
       Student(int id, const std::string& n, int a)
           : student_id_(id), name_(n), age_(a) {}
   };

**Just 10-15 lines!** The macro generates all the necessary code automatically.

Basic Usage
-----------

The LEARNQL_PROPERTIES System
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The property system uses two macros that work together:

.. code-block:: cpp

   LEARNQL_PROPERTIES_BEGIN(ClassName)
       LEARNQL_PROPERTY(Type, name, [PK])
       LEARNQL_PROPERTY(Type, name)
       // ... more properties
   LEARNQL_PROPERTIES_END(
       PROP(Type, name, [PK]),
       PROP(Type, name),
       // ... matching PROP entries
   )

**Key Points:**

* ``LEARNQL_PROPERTIES_BEGIN`` starts the property section
* Each property is declared twice: once with ``LEARNQL_PROPERTY`` and once with ``PROP``
* The first property should be marked with ``PK`` (Primary Key)
* ``LEARNQL_PROPERTIES_END`` closes the section

Complete Example
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   class Person {
       LEARNQL_PROPERTIES_BEGIN(Person)
           LEARNQL_PROPERTY(int, person_id, PK)
           LEARNQL_PROPERTY(std::string, first_name)
           LEARNQL_PROPERTY(std::string, last_name)
           LEARNQL_PROPERTY(int, age)
           LEARNQL_PROPERTY(double, salary)
       LEARNQL_PROPERTIES_END(
           PROP(int, person_id, PK),
           PROP(std::string, first_name),
           PROP(std::string, last_name),
           PROP(int, age),
           PROP(double, salary)
       )

   public:
       Person() = default;
   };

Accessing Properties
~~~~~~~~~~~~~~~~~~~~

The macro system generates getters and setters automatically:

.. code-block:: cpp

   Person person;

   // Use setters to modify values
   person.set_person_id(1);
   person.set_first_name("Alice");
   person.set_last_name("Johnson");
   person.set_age(30);
   person.set_salary(75000.0);

   // Use getters to read values
   std::cout << "ID: " << person.get_person_id() << std::endl;
   std::cout << "Name: " << person.get_first_name() << " "
             << person.get_last_name() << std::endl;
   std::cout << "Age: " << person.get_age() << std::endl;
   std::cout << "Salary: $" << person.get_salary() << std::endl;

Constructor Initialization
~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can create convenience constructors that initialize the private member variables directly:

.. code-block:: cpp

   class Person {
       LEARNQL_PROPERTIES_BEGIN(Person)
           LEARNQL_PROPERTY(int, person_id, PK)
           LEARNQL_PROPERTY(std::string, name)
           LEARNQL_PROPERTY(int, age)
       LEARNQL_PROPERTIES_END(
           PROP(int, person_id, PK),
           PROP(std::string, name),
           PROP(int, age)
       )

   public:
       Person() = default;

       // Convenience constructor initializes private members
       Person(int id, const std::string& n, int a)
           : person_id_(id), name_(n), age_(a) {}
   };

   // Usage
   Person alice(1, "Alice Johnson", 30);
   std::cout << alice.get_name() << std::endl;  // "Alice Johnson"

.. note::
   Member variables have a trailing underscore (``person_id_``, ``name_``, ``age_``). This is the naming convention used by the macro system.

Supported Property Types
------------------------

Primitive Types
~~~~~~~~~~~~~~~

All standard C++ primitive types are supported:

.. code-block:: cpp

   class Example {
       LEARNQL_PROPERTIES_BEGIN(Example)
           LEARNQL_PROPERTY(int, id, PK)
           LEARNQL_PROPERTY(char, letter)
           LEARNQL_PROPERTY(short, small_number)
           LEARNQL_PROPERTY(long, big_number)
           LEARNQL_PROPERTY(float, decimal)
           LEARNQL_PROPERTY(double, precise_decimal)
           LEARNQL_PROPERTY(bool, flag)
       LEARNQL_PROPERTIES_END(
           PROP(int, id, PK),
           PROP(char, letter),
           PROP(short, small_number),
           PROP(long, big_number),
           PROP(float, decimal),
           PROP(double, precise_decimal),
           PROP(bool, flag)
       )
   };

String Types
~~~~~~~~~~~~

.. code-block:: cpp

   class Person {
       LEARNQL_PROPERTIES_BEGIN(Person)
           LEARNQL_PROPERTY(int, person_id, PK)
           LEARNQL_PROPERTY(std::string, name)
           LEARNQL_PROPERTY(std::string, email)
           LEARNQL_PROPERTY(std::string, address)
       LEARNQL_PROPERTIES_END(
           PROP(int, person_id, PK),
           PROP(std::string, name),
           PROP(std::string, email),
           PROP(std::string, address)
       )
   };

What the Macros Generate
-------------------------

Generated Members
~~~~~~~~~~~~~~~~~

For each ``LEARNQL_PROPERTY(Type, name)``, the macro system generates:

1. **Private member variable**: ``Type name_;``
2. **Getter method**: ``Type get_name() const`` (or ``const Type&`` for strings)
3. **Setter method**: ``void set_name(Type value)`` (or ``const Type&`` for strings)
4. **Static Field object**: ``static inline const Field<ClassName, Type> name``

Example: What Gets Generated
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   LEARNQL_PROPERTY(int, age)

   // Approximately expands to:
   private:
       int age_;

   public:
       [[nodiscard]] int get_age() const { return age_; }
       void set_age(int value) { age_ = value; }

       static inline const Field<Student, int> age{
           "age",
           [](const Student& s) { return s.get_age(); }
       };

Static Field Objects
~~~~~~~~~~~~~~~~~~~~

The static Field objects are used in query expressions:

.. code-block:: cpp

   class Student {
       LEARNQL_PROPERTIES_BEGIN(Student)
           LEARNQL_PROPERTY(int, student_id, PK)
           LEARNQL_PROPERTY(int, age)
           LEARNQL_PROPERTY(double, gpa)
       LEARNQL_PROPERTIES_END(
           PROP(int, student_id, PK),
           PROP(int, age),
           PROP(double, gpa)
       )
   };

   // Generated static Field objects:
   // Student::student_id (type: Field<Student, int>)
   // Student::age (type: Field<Student, int>)
   // Student::gpa (type: Field<Student, double>)

   // Use in queries:
   auto adults = students.where(Student::age >= 21);
   auto high_achievers = students.where(Student::gpa > 3.5);

Advanced Patterns
-----------------

Primary Keys
~~~~~~~~~~~~

The **first property** marked with ``PK`` is automatically the primary key:

.. code-block:: cpp

   class Student {
       LEARNQL_PROPERTIES_BEGIN(Student)
           LEARNQL_PROPERTY(int, student_id, PK)  // ← PRIMARY KEY
           LEARNQL_PROPERTY(std::string, name)
           LEARNQL_PROPERTY(int, age)
       LEARNQL_PROPERTIES_END(
           PROP(int, student_id, PK),
           PROP(std::string, name),
           PROP(int, age)
       )
   };

Rules for primary keys:

* Must be unique across all records
* Used for indexing and fast lookups
* Typically an integer type for best performance
* Cannot be changed after insertion (treat as immutable)

Composite Models
~~~~~~~~~~~~~~~~

You can create complex classes with many properties:

.. code-block:: cpp

   class Employee {
       LEARNQL_PROPERTIES_BEGIN(Employee)
           // Identification
           LEARNQL_PROPERTY(int, employee_id, PK)
           LEARNQL_PROPERTY(std::string, employee_number)

           // Personal info
           LEARNQL_PROPERTY(std::string, first_name)
           LEARNQL_PROPERTY(std::string, last_name)
           LEARNQL_PROPERTY(std::string, email)
           LEARNQL_PROPERTY(std::string, phone)

           // Employment info
           LEARNQL_PROPERTY(std::string, department)
           LEARNQL_PROPERTY(std::string, position)
           LEARNQL_PROPERTY(double, salary)
           LEARNQL_PROPERTY(bool, is_full_time)
       LEARNQL_PROPERTIES_END(
           PROP(int, employee_id, PK),
           PROP(std::string, employee_number),
           PROP(std::string, first_name),
           PROP(std::string, last_name),
           PROP(std::string, email),
           PROP(std::string, phone),
           PROP(std::string, department),
           PROP(std::string, position),
           PROP(double, salary),
           PROP(bool, is_full_time)
       )

   public:
       Employee() = default;
   };

Custom Methods
~~~~~~~~~~~~~~

You can add custom methods alongside properties:

.. code-block:: cpp

   class Person {
       LEARNQL_PROPERTIES_BEGIN(Person)
           LEARNQL_PROPERTY(int, person_id, PK)
           LEARNQL_PROPERTY(std::string, first_name)
           LEARNQL_PROPERTY(std::string, last_name)
           LEARNQL_PROPERTY(int, age)
       LEARNQL_PROPERTIES_END(
           PROP(int, person_id, PK),
           PROP(std::string, first_name),
           PROP(std::string, last_name),
           PROP(int, age)
       )

   public:
       Person() = default;

       // Custom helper methods
       std::string full_name() const {
           return get_first_name() + " " + get_last_name();
       }

       bool is_adult() const {
           return get_age() >= 18;
       }

       bool is_senior() const {
           return get_age() >= 65;
       }
   };

   // Usage
   Person person(1, "Alice", "Johnson", 25);
   std::cout << person.full_name() << std::endl;  // "Alice Johnson"
   if (person.is_adult()) {
       std::cout << "Is an adult" << std::endl;
   }

Integration with Query DSL
---------------------------

Properties and Static Fields
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The property system integrates seamlessly with LearnQL's query DSL through static Field objects:

.. code-block:: cpp

   students.where(Student::age >= 21);
   //             ↑
   //             Static Field object auto-generated by LEARNQL_PROPERTIES

How it works:

1. ``LEARNQL_PROPERTIES`` generates ``Student::age`` as a static Field object
2. ``>= 21`` creates a typed comparison expression
3. The whole expression is type-checked at compile-time
4. No runtime overhead!

Type Safety Example
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   class Student {
       LEARNQL_PROPERTIES_BEGIN(Student)
           LEARNQL_PROPERTY(int, student_id, PK)
           LEARNQL_PROPERTY(std::string, name)
           LEARNQL_PROPERTY(int, age)
           LEARNQL_PROPERTY(double, gpa)
       LEARNQL_PROPERTIES_END(
           PROP(int, student_id, PK),
           PROP(std::string, name),
           PROP(int, age),
           PROP(double, gpa)
       )
   };

   auto& students = db.table<Student>("students");

   // ✓ Valid: comparing int with int
   students.where(Student::age >= 21);

   // ✓ Valid: comparing string with string
   students.where(Student::name == "Alice");

   // ✓ Valid: comparing double with double
   students.where(Student::gpa > 3.5);

   // ✗ Compile error: comparing string with int
   students.where(Student::name >= 21);  // Type mismatch!

   // ✗ Compile error: no such property
   students.where(Student::invalid >= 21);  // Property doesn't exist!

Common Pitfalls and Solutions
------------------------------

Pitfall 1: Mismatched Property Declarations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

❌ **Wrong:**

.. code-block:: cpp

   class Student {
       LEARNQL_PROPERTIES_BEGIN(Student)
           LEARNQL_PROPERTY(int, id, PK)
           LEARNQL_PROPERTY(std::string, name)
       LEARNQL_PROPERTIES_END(
           PROP(int, id, PK),
           // Missing PROP for name!
       )
   };

✅ **Correct:**

.. code-block:: cpp

   class Student {
       LEARNQL_PROPERTIES_BEGIN(Student)
           LEARNQL_PROPERTY(int, id, PK)
           LEARNQL_PROPERTY(std::string, name)
       LEARNQL_PROPERTIES_END(
           PROP(int, id, PK),
           PROP(std::string, name)  // Must match!
       )
   };

Pitfall 2: Forgetting Default Constructor
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

❌ **Wrong:**

.. code-block:: cpp

   class Student {
       LEARNQL_PROPERTIES_BEGIN(Student)
           LEARNQL_PROPERTY(int, id, PK)
       LEARNQL_PROPERTIES_END(
           PROP(int, id, PK)
       )
       // No default constructor - will cause errors!
   };

✅ **Correct:**

.. code-block:: cpp

   class Student {
       LEARNQL_PROPERTIES_BEGIN(Student)
           LEARNQL_PROPERTY(int, id, PK)
       LEARNQL_PROPERTIES_END(
           PROP(int, id, PK)
       )

   public:
       Student() = default;  // Required!
   };

Pitfall 3: Direct Member Access
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

❌ **Wrong:**

.. code-block:: cpp

   Student student;
   student.name_ = "Alice";  // Private member - won't compile!

✅ **Correct:**

.. code-block:: cpp

   Student student;
   student.set_name("Alice");  // Use setter!

Pitfall 4: Changing Primary Key
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

❌ **Wrong:**

.. code-block:: cpp

   Student s(1, "Alice", 20);
   students.insert(s);

   s.set_student_id(2);  // Don't change primary key!
   students.update(s);    // Database now inconsistent!

✅ **Solution:** Treat primary keys as immutable. Delete and re-insert if needed:

.. code-block:: cpp

   students.remove(1);
   s.set_student_id(2);
   students.insert(s);

Best Practices
--------------

1. **Use Meaningful Names**

   .. code-block:: cpp

      // Good
      LEARNQL_PROPERTY(std::string, email_address)

      // Bad
      LEARNQL_PROPERTY(std::string, ea)

2. **Choose Appropriate Primary Keys**

   .. code-block:: cpp

      // Good: Integer primary key (fast!)
      LEARNQL_PROPERTY(int, student_id, PK)

      // Acceptable but slower: String primary key
      LEARNQL_PROPERTY(std::string, student_id, PK)

3. **Keep Classes Flat**

   .. code-block:: cpp

      // Good: Flat structure
      class Person {
          LEARNQL_PROPERTIES_BEGIN(Person)
              LEARNQL_PROPERTY(int, person_id, PK)
              LEARNQL_PROPERTY(std::string, street)
              LEARNQL_PROPERTY(std::string, city)
          LEARNQL_PROPERTIES_END(/* ... */)
      };

      // Not supported: Nested objects
      class Address {
          std::string street;
          std::string city;
      };

      class Person {
          LEARNQL_PROPERTY(Address, address)  // Won't work!
      };

      // Solution: Use foreign keys or flatten
      class Person {
          LEARNQL_PROPERTY(int, person_id, PK)
          LEARNQL_PROPERTY(int, address_id)  // Foreign key
      };

4. **Always Provide Default Constructor**

   .. code-block:: cpp

      class Student {
          LEARNQL_PROPERTIES_BEGIN(Student)
              // ... properties
          LEARNQL_PROPERTIES_END(/* ... */)

      public:
          Student() = default;  // Required!
      };

Performance Benefits
--------------------

The property macro system provides:

**Compile-Time Benefits:**
* Zero runtime overhead
* Type checking during compilation
* Compile-time optimization opportunities

**Development Benefits:**
* ~70% reduction in boilerplate code
* Consistent naming conventions
* Fewer opportunities for bugs

**Runtime Benefits:**
* Efficient getter/setter inlining
* Optimized serialization
* Fast Field object lookups

Next Steps
----------

Now that you understand property macros:

* :doc:`best-practices` - Learn best practices for using LearnQL
* :doc:`/tutorials/tutorial-01-first-database` - Build a complete application
* :doc:`/api/reflection` - Explore the reflection system API
* :doc:`/architecture/reflection-system` - Understand how the macro system works internally

.. tip::
   Property macros are what make LearnQL both powerful and easy to use. Take time to understand them well - they're the foundation of everything else!
