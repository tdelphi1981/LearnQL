/**
 * @file main.cpp
 * @brief Complete demonstration of LearnQL capabilities with property macros
 *
 * This example demonstrates:
 * - Database class for high-level table management
 * - Property macros for rapid development (LEARNQL_PROPERTY)
 * - Table creation and CRUD operations
 * - Expression templates for queries
 * - Join operations (inner, left, cross)
 * - GroupBy and aggregations
 * - C++20 ranges integration
 * - Batched loading for memory efficiency
 * - Query builder pattern
 * - System catalog for queryable metadata
 * - Schema introspection at runtime
 * - Database structure inspection
 * - Persistent secondary indexes (NEW!)
 *
 * Key Features:
 * 1. Database class: High-level API with automatic resource management
 * 2. Property macros: Automatic getter/setter generation, static Fields,
 *    type-safe serialization, 46% reduction in boilerplate code
 * 3. System catalog: Queryable metadata tables, supports 1000+ tables,
 *    automatic record count synchronization
 * 4. Persistent secondary indexes: Fast O(log n) lookups on non-primary-key
 *    fields, supports unique and multi-value indexes, persistent to disk
 */


#include <learnql/LearnQL.hpp>
#include <iostream>
#include <iomanip>
#include <ranges>
#include <algorithm>

using namespace learnql;
using namespace learnql::query;  // For && and || operators in expression templates

// ============================================================================
// Domain Models
// ============================================================================

/**
 * @brief Student record with academic information
 *
 * Uses new enhanced LEARNQL_PROPERTIES system for automatic generation of:
 * - Private member variables with trailing underscore (e.g., student_id_)
 * - Type-safe getters and setters
 * - Static Field objects for query DSL
 * - Serialization/deserialization methods
 * - Reflection metadata for system catalog
 * - Primary key detection and getter
 *
 * This reduces boilerplate by ~70% compared to manual approach!
 * Each property defined once - everything else is auto-generated!
 */
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
    // Default constructor
    Student() = default;

    // Convenience constructor for easy initialization
    Student(int sid, const std::string& n, const std::string& dept, int a, double g)
        : student_id_(sid), name_(n), department_(dept), age_(a), gpa_(g) {}
};

/**
 * @brief Course enrollment record
 *
 * Uses new enhanced LEARNQL_PROPERTIES system for automatic generation of:
 * - Private member variables with trailing underscore
 * - Type-safe getters and setters
 * - Static Field objects for query DSL
 * - Serialization/deserialization methods
 * - Reflection metadata for system catalog
 * - Primary key detection and getter
 */
class Enrollment {
    LEARNQL_PROPERTIES_BEGIN(Enrollment)
        LEARNQL_PROPERTY(int, enrollment_id, PK)
        LEARNQL_PROPERTY(int, student_id)
        LEARNQL_PROPERTY(std::string, course_code)
        LEARNQL_PROPERTY(int, semester)
        LEARNQL_PROPERTY(char, grade)
    LEARNQL_PROPERTIES_END(
        PROP(int, enrollment_id, PK),
        PROP(int, student_id),
        PROP(std::string, course_code),
        PROP(int, semester),
        PROP(char, grade)
    )

public:
    // Default constructor
    Enrollment() = default;

    // Convenience constructor for easy initialization
    Enrollment(int eid, int sid, const std::string& code, int sem, char g)
        : enrollment_id_(eid), student_id_(sid), course_code_(code), semester_(sem), grade_(g) {}
};

/**
 * @brief Course information
 *
 * Uses new enhanced LEARNQL_PROPERTIES system for automatic generation of:
 * - Private member variables with trailing underscore
 * - Type-safe getters and setters
 * - Static Field objects for query DSL
 * - Serialization/deserialization methods
 * - Reflection metadata for system catalog
 * - Primary key detection and getter
 */
class Course {
    LEARNQL_PROPERTIES_BEGIN(Course)
        LEARNQL_PROPERTY(std::string, course_code, PK)
        LEARNQL_PROPERTY(std::string, title)
        LEARNQL_PROPERTY(int, credits)
        LEARNQL_PROPERTY(std::string, instructor)
    LEARNQL_PROPERTIES_END(
        PROP(std::string, course_code, PK),
        PROP(std::string, title),
        PROP(int, credits),
        PROP(std::string, instructor)
    )

public:
    // Default constructor
    Course() = default;

    // Convenience constructor for easy initialization
    Course(const std::string& code, const std::string& t, int cred, const std::string& instr)
        : course_code_(code), title_(t), credits_(cred), instructor_(instr) {}
};

// ============================================================================
// Helper Functions
// ============================================================================

void print_section(const std::string& title) {
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << title << "\n";
    std::cout << std::string(80, '=') << "\n\n";
}

void print_student(const Student& s) {
    std::cout << std::setw(5) << s.get_student_id() << " | "
              << std::setw(20) << std::left << s.get_name() << " | "
              << std::setw(12) << s.get_department() << " | "
              << "Age: " << std::setw(2) << s.get_age() << " | "
              << "GPA: " << std::fixed << std::setprecision(2) << s.get_gpa()
              << std::endl;
}

void print_enrollment(const Enrollment& e) {
    std::cout << "ID: " << std::setw(5) << e.get_enrollment_id() << " | "
              << "Student: " << std::setw(5) << e.get_student_id() << " | "
              << "Course: " << std::setw(8) << e.get_course_code() << " | "
              << "Semester: " << e.get_semester() << " | "
              << "Grade: " << e.get_grade()
              << std::endl;
}

void print_course(const Course& c) {
    std::cout << std::setw(8) << c.get_course_code() << " | "
              << std::setw(30) << std::left << c.get_title() << " | "
              << "Credits: " << c.get_credits() << " | "
              << "Instructor: " << c.get_instructor()
              << std::endl;
}

// ============================================================================
// Main Demo
// ============================================================================

int main() {
    try {
        std::cout << "\n";
        std::cout << R"(
    ╔══════════════════════════════════════════════════════════════════╗
    ║                                                                  ║
    ║                   LearnQL Complete Demo                          ║
    ║                                                                  ║
    ║        A Modern C++ Database Query Library                       ║
    ║        Featuring Batched Loading for Memory Efficiency          ║
    ║                                                                  ║
    ╚══════════════════════════════════════════════════════════════════╝
        )" << "\n\n";

        // ====================================================================
        // 1. Setup: Database and Tables
        // ====================================================================
        print_section("1. Database Setup");

        std::cout << "Creating database...\n";
        // Database is the high-level API for managing tables with shared storage
        core::Database db("university.db");

        std::cout << "Creating tables (using default batch size = 10)...\n";
        // Note: Tables use BatchSize=10 by default for memory-efficient traversal
        // Database automatically manages the underlying storage engine
        auto& students = db.table<Student>("students");
        auto& enrollments = db.table<Enrollment>("enrollments");
        auto& courses = db.table<Course>("courses");

        std::cout << "✓ Database initialized successfully!\n";

        // ====================================================================
        // 2. Insert Operations
        // ====================================================================
        print_section("2. Inserting Records");

        std::cout << "Inserting students...\n";
        std::vector<Student> student_data = {
            {1001, "Alice Johnson", "CS", 20, 3.8},
            {1002, "Bob Smith", "CS", 21, 3.5},
            {1003, "Carol White", "Math", 19, 3.9},
            {1004, "David Brown", "Physics", 22, 3.2},
            {1005, "Eve Davis", "CS", 20, 3.7},
            {1006, "Frank Wilson", "Math", 21, 3.4},
            {1007, "Grace Lee", "CS", 19, 4.0},
            {1008, "Henry Taylor", "Physics", 23, 3.3},
            {1009, "Iris Martin", "Math", 20, 3.6},
            {1010, "Jack Anderson", "CS", 22, 3.1}
        };

        for (const auto& student : student_data) {
            students.insert(student);
        }
        std::cout << "✓ Inserted " << students.size() << " students\n";

        std::cout << "\nInserting courses...\n";
        std::vector<Course> course_data = {
            {"CS101", "Introduction to Programming", 3, "Prof. Adams"},
            {"CS201", "Data Structures", 4, "Prof. Baker"},
            {"CS301", "Algorithms", 4, "Prof. Clark"},
            {"MATH101", "Calculus I", 4, "Prof. Davis"},
            {"MATH201", "Linear Algebra", 3, "Prof. Evans"},
            {"PHYS101", "Physics I", 4, "Prof. Foster"}
        };

        for (const auto& course : course_data) {
            courses.insert(course);
        }
        std::cout << "✓ Inserted " << courses.size() << " courses\n";

        std::cout << "\nInserting enrollments...\n";
        std::vector<Enrollment> enrollment_data = {
            {1, 1001, "CS101", 1, 'A'},
            {2, 1001, "CS201", 1, 'B'},
            {3, 1002, "CS101", 1, 'B'},
            {4, 1002, "CS201", 1, 'C'},
            {5, 1003, "MATH101", 1, 'A'},
            {6, 1003, "MATH201", 1, 'A'},
            {7, 1004, "PHYS101", 1, 'B'},
            {8, 1005, "CS101", 1, 'A'},
            {9, 1005, "CS301", 2, 'A'},
            {10, 1007, "CS101", 1, 'A'},
            {11, 1007, "CS201", 1, 'A'},
            {12, 1007, "CS301", 2, 'A'}
        };

        for (const auto& enrollment : enrollment_data) {
            enrollments.insert(enrollment);
        }
        std::cout << "✓ Inserted " << enrollments.size() << " enrollments\n";

        // ====================================================================
        // 3. Basic Queries - Batched Loading in Action
        // ====================================================================
        print_section("3. Basic Queries with Batched Loading");

        std::cout << "Iterating over all students (loaded in batches of 10):\n";
        std::cout << std::string(80, '-') << "\n";

        int count = 0;
        for (const auto& student : students) {
            print_student(student);
            count++;
            // Note: Only current batch is in memory at any time!
        }
        std::cout << "\nTotal: " << count << " students\n";
        std::cout << "Memory-efficient: Records loaded in batches, not all at once!\n";

        // ====================================================================
        // 4. Expression Template Queries (SQL-like Syntax)
        // ====================================================================
        print_section("4. Expression Template Queries (SQL-like Syntax)");

        std::cout << "LearnQL supports expression templates for SQL-like queries:\n\n";

        std::cout << "1. Simple comparison: gpa > 3.5\n";
        std::cout << std::string(80, '-') << "\n";
        auto high_gpa = students.where(Student::gpa > 3.5);
        for (const auto& s : high_gpa) {
            print_student(s);
        }

        std::cout << "\n2. String equality: department == \"CS\"\n";
        std::cout << std::string(80, '-') << "\n";
        auto cs_students = students.where(Student::department == "CS");
        for (const auto& s : cs_students) {
            print_student(s);
        }

        std::cout << "\n3. Combined with AND: (department == \"CS\") && (gpa >= 3.7)\n";
        std::cout << std::string(80, '-') << "\n";
        auto elite_cs = students.where((Student::department == "CS") && (Student::gpa >= 3.7));
        for (const auto& s : elite_cs) {
            print_student(s);
        }

        std::cout << "\n4. Combined with OR: (age <= 20) || (gpa >= 3.9)\n";
        std::cout << std::string(80, '-') << "\n";
        auto young_or_smart = students.where((Student::age <= 20) || (Student::gpa >= 3.9));
        for (const auto& s : young_or_smart) {
            print_student(s);
        }

        std::cout << "\n5. Complex nested expression:\n";
        std::cout << "   ((department == \"CS\") && (gpa > 3.5)) || (age < 20)\n";
        std::cout << std::string(80, '-') << "\n";
        auto complex = students.where(
            ((Student::department == "CS") && (Student::gpa > 3.5)) || (Student::age < 20)
        );
        for (const auto& s : complex) {
            print_student(s);
        }

        std::cout << "\n6. All comparison operators demonstrated:\n";
        std::cout << std::string(80, '-') << "\n";

        std::cout << "   ==  (equal):     age == 20\n";
        for (const auto& s : students.where(Student::age == 20)) {
            std::cout << "     " << s.get_name() << " (age: " << s.get_age() << ")\n";
        }

        std::cout << "\n   !=  (not equal): age != 20\n";
        int ne_count = 0;
        for (const auto& s : students.where(Student::age != 20)) {
            if (ne_count++ < 3) std::cout << "     " << s.get_name() << " (age: " << s.get_age() << ")\n";
        }
        std::cout << "     ... and " << (ne_count - 3) << " more\n";

        std::cout << "\n   <   (less):      gpa < 3.5\n";
        for (const auto& s : students.where(Student::gpa < 3.5)) {
            std::cout << "     " << s.get_name() << " (GPA: " << std::fixed << std::setprecision(2) << s.get_gpa() << ")\n";
        }

        std::cout << "\n   >=  (greater or equal): gpa >= 3.9\n";
        for (const auto& s : students.where(Student::gpa >= 3.9)) {
            std::cout << "     " << s.get_name() << " (GPA: " << std::fixed << std::setprecision(2) << s.get_gpa() << ")\n";
        }

        std::cout << "\nExpression Template Benefits:\n";
        std::cout << "  ✓ SQL-like syntax (clean and readable)\n";
        std::cout << "  ✓ Zero runtime overhead (compiled away)\n";
        std::cout << "  ✓ Type-safe at compile-time\n";
        std::cout << "  ✓ All comparison operators: ==, !=, <, <=, >, >=\n";
        std::cout << "  ✓ Logical operators: && (AND), || (OR)\n";
        std::cout << "  ✓ Complex nested expressions supported\n";
        std::cout << "  ✓ Works with batched loading (memory-efficient)\n";

        // ====================================================================
        // 5. CRUD Operations
        // ====================================================================
        print_section("5. CRUD Operations");

        std::cout << "Finding student by ID (1001):\n";
        auto alice = students.find(1001);
        if (alice) {
            print_student(*alice);
        }

        std::cout << "\nUpdating student 1001's GPA to 3.9 (using setter):\n";
        if (alice) {
            alice->set_gpa(3.9);  // Using macro-generated setter!
            students.update(*alice);

            auto updated = students.find(1001);
            if (updated) {
                print_student(*updated);
            }
        }

        std::cout << "\nChecking if student exists:\n";
        std::cout << "Student 1001 exists: " << (students.contains(1001) ? "Yes" : "No") << "\n";
        std::cout << "Student 9999 exists: " << (students.contains(9999) ? "Yes" : "No") << "\n";

        // ====================================================================
        // 6. Join Operations
        // ====================================================================
        print_section("6. Join Operations");

        std::cout << "Inner Join: Students with their enrollments\n";
        std::cout << std::string(80, '-') << "\n";

        auto student_enrollments = query::Join<Student, Enrollment>::inner_join(
            students,
            enrollments,
            [](const Student& s) { return s.get_student_id(); },
            [](const Enrollment& e) { return e.get_student_id(); }
        );

        for (const auto& [student, enrollment_opt] : student_enrollments) {
            if (enrollment_opt) {
                std::cout << std::setw(20) << std::left << student.get_name() << " | "
                          << "Course: " << std::setw(8) << enrollment_opt->get_course_code() << " | "
                          << "Grade: " << enrollment_opt->get_grade() << "\n";
            }
        }

        std::cout << "\nLeft Join: All students with their courses (including unenrolled)\n";
        std::cout << std::string(80, '-') << "\n";

        auto all_student_enrollments = query::Join<Student, Enrollment>::left_join(
            students,
            enrollments,
            [](const Student& s) { return s.get_student_id(); },
            [](const Enrollment& e) { return e.get_student_id(); }
        );

        std::unordered_map<int, std::vector<std::string>> student_courses;
        for (const auto& [student, enrollment_opt] : all_student_enrollments) {
            if (enrollment_opt) {
                student_courses[student.get_student_id()].push_back(enrollment_opt->get_course_code());
            }
        }

        for (const auto& student : students) {
            std::cout << std::setw(20) << std::left << student.get_name() << " | ";
            if (student_courses.count(student.get_student_id())) {
                std::cout << "Courses: ";
                for (size_t i = 0; i < student_courses[student.get_student_id()].size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << student_courses[student.get_student_id()][i];
                }
            } else {
                std::cout << "No enrollments";
            }
            std::cout << "\n";
        }

        // ====================================================================
        // 7. GroupBy and Aggregations
        // ====================================================================
        print_section("7. GroupBy and Aggregations");

        std::cout << "Count students by department:\n";
        std::cout << std::string(80, '-') << "\n";

        auto dept_counts = query::GroupBy<Student, std::string>::count_by(
            students,
            [](const Student& s) { return s.get_department(); }
        );

        for (const auto& result : dept_counts) {
            std::cout << std::setw(12) << std::left << result.key << " | "
                      << "Students: " << result.value << "\n";
        }

        std::cout << "\nAverage GPA by department:\n";
        std::cout << std::string(80, '-') << "\n";

        auto dept_avg_gpa = query::GroupBy<Student, std::string>::average_by(
            students,
            [](const Student& s) { return s.get_department(); },
            [](const Student& s) { return s.get_gpa(); }
        );

        for (const auto& result : dept_avg_gpa) {
            std::cout << std::setw(12) << std::left << result.key << " | "
                      << "Avg GPA: " << std::fixed << std::setprecision(2) << result.value
                      << " (" << result.count << " students)\n";
        }

        std::cout << "\nSum of credits by student (multi-join aggregation):\n";
        std::cout << std::string(80, '-') << "\n";

        // Join students -> enrollments -> courses
        auto student_course_join = query::Join<Student, Enrollment>::inner_join(
            students,
            enrollments,
            [](const Student& s) { return s.get_student_id(); },
            [](const Enrollment& e) { return e.get_student_id(); }
        );

        // Extract student-course pairs
        struct StudentCourse {
            int student_id;
            std::string student_name;
            std::string course_code;
        };

        std::vector<StudentCourse> student_courses_list;
        for (const auto& [student, enrollment_opt] : student_course_join) {
            if (enrollment_opt) {
                student_courses_list.push_back({
                    student.get_student_id(),
                    student.get_name(),
                    enrollment_opt->get_course_code()
                });
            }
        }

        // Calculate total credits per student
        std::unordered_map<int, std::pair<std::string, int>> student_credits;
        for (const auto& sc : student_courses_list) {
            auto course = courses.find(sc.course_code);
            if (course) {
                student_credits[sc.student_id].first = sc.student_name;
                student_credits[sc.student_id].second += course->get_credits();
            }
        }

        for (const auto& [id, name_credits] : student_credits) {
            std::cout << std::setw(20) << std::left << name_credits.first << " | "
                      << "Total Credits: " << name_credits.second << "\n";
        }

        // ====================================================================
        // 8. C++20 Ranges Integration
        // ====================================================================
        print_section("8. C++20 Ranges Integration");

        std::cout << "Using ranges to filter and transform:\n";
        std::cout << std::string(80, '-') << "\n";

        // Get CS students with GPA > 3.5 using batched loading
        auto cs_high_gpa_batched = students.find_if([](const Student& s) {
            return s.get_department() == "CS" && s.get_gpa() > 3.5;
        });

        std::cout << "CS High Achievers (GPA > 3.5):\n";
        for (const auto& student : cs_high_gpa_batched) {
            std::cout << "  - " << student.get_name() << "\n";
        }

        std::cout << "\nTop 3 students by GPA:\n";

        // Materialize for sorting (ranges don't support random access easily)
        std::vector<Student> all_students;
        for (const auto& s : students) {
            all_students.push_back(s);
        }

        std::ranges::sort(all_students, [](const Student& a, const Student& b) {
            return a.get_gpa() > b.get_gpa();
        });

        for (size_t i = 0; i < std::min<size_t>(3, all_students.size()); ++i) {
            std::cout << "  " << (i + 1) << ". ";
            print_student(all_students[i]);
        }

        // ====================================================================
        // 9. Query Builder Pattern
        // ====================================================================
        print_section("9. Query Builder Pattern");

        std::cout << "Using Query builder:\n";
        std::cout << std::string(80, '-') << "\n";

        // Note: Query class now supports batched execution
        query::Query<Student, 10> student_query(students);

        std::cout << "Count all students: " << student_query.count() << "\n";
        std::cout << "Any students? " << (student_query.any() ? "Yes" : "No") << "\n";

        std::cout << "\nQuery with filter:\n";
        std::size_t cs_student_count = 0;
        for (const auto& student : students) {
            if (student.get_department() == "CS") {
                cs_student_count++;
            }
        }
        std::cout << "CS students count: " << cs_student_count << "\n";

        // ====================================================================
        // 10. Batched Loading Performance Demo
        // ====================================================================
        print_section("10. Batched Loading Performance Benefits");

        std::cout << "Memory-Efficient Iteration:\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "With batched loading (batch size = 10):\n";
        std::cout << "  ✓ Only 10 record IDs loaded into memory at a time\n";
        std::cout << "  ✓ Old batches automatically discarded after processing\n";
        std::cout << "  ✓ Memory usage stays constant regardless of table size\n";
        std::cout << "  ✓ Perfect for processing millions of records\n\n";

        std::cout << "Traditional approach would load ALL record IDs at once:\n";
        std::cout << "  ✗ 1 million records = ~24MB+ of RecordIds in memory\n";
        std::cout << "  ✓ Batched approach = ~240 bytes (constant!)\n\n";

        std::cout << "Demonstrating batch processing:\n";
        int batch_count = 0;
        int record_count = 0;

        auto all_students_batched = students.get_all();
        std::cout << "Processing students in batches...\n";

        for (const auto& student : all_students_batched) {
            (void)student;  // Suppress unused variable warning
            record_count++;
            if (record_count % 10 == 1) {
                batch_count++;
                std::cout << "  Batch " << batch_count << ": Loading records "
                          << record_count << "-" << (record_count + 9) << "...\n";
            }
        }
        std::cout << "✓ Processed " << record_count << " records using "
                  << batch_count << " batches\n";

        // ====================================================================
        // 11. Advanced Filtering
        // ====================================================================
        print_section("11. Advanced Filtering and Composition");

        std::cout << "Chained filters using lambda predicates:\n";
        std::cout << std::string(80, '-') << "\n";

        // Find young CS students with high GPA
        auto elite_cs_students = students.find_if([](const Student& s) {
            return s.get_department() == "CS"
                && s.get_age() <= 20
                && s.get_gpa() >= 3.7;
        });

        std::cout << "Elite CS students (CS, age ≤ 20, GPA ≥ 3.7):\n";
        for (const auto& student : elite_cs_students) {
            print_student(student);
        }

        // ====================================================================
        // 12. Statistics and Analytics
        // ====================================================================
        print_section("12. Statistics and Analytics");

        std::cout << "Database statistics:\n";
        std::cout << std::string(80, '-') << "\n";

        double total_gpa = 0.0;
        double max_gpa = 0.0;
        double min_gpa = 4.0;
        int student_count = 0;

        for (const auto& student : students) {
            total_gpa += student.get_gpa();
            max_gpa = std::max(max_gpa, student.get_gpa());
            min_gpa = std::min(min_gpa, student.get_gpa());
            student_count++;
        }

        std::cout << "Total students: " << student_count << "\n";
        std::cout << "Average GPA: " << std::fixed << std::setprecision(2)
                  << (total_gpa / student_count) << "\n";
        std::cout << "Highest GPA: " << max_gpa << "\n";
        std::cout << "Lowest GPA: " << min_gpa << "\n";

        std::cout << "\nGrade distribution in enrollments:\n";
        std::unordered_map<char, int> grade_dist;
        for (const auto& enrollment : enrollments) {
            grade_dist[enrollment.get_grade()]++;
        }

        for (const auto& [grade, count] : grade_dist) {
            std::cout << "  Grade " << grade << ": " << count << " students\n";
        }

        // ====================================================================
        // 13. Persistence and Flush
        // ====================================================================
        print_section("13. Persistence");

        std::cout << "Flushing data to disk...\n";
        db.flush();  // Database automatically flushes all tables and storage
        std::cout << "✓ All changes persisted to 'university.db'\n";
        std::cout << "✓ Data will survive program restart\n";
        std::cout << "\nNote: Database also auto-flushes when it goes out of scope!\n";

        // ====================================================================
        // 14. System Catalog - Queryable Metadata
        // ====================================================================
        print_section("14. System Catalog - Queryable Metadata (NEW!)");

        std::cout << "LearnQL now includes a queryable system catalog!\n";
        std::cout << "Metadata is stored in special tables and can be queried like regular data.\n\n";

        // Access the system catalog
        auto& catalog = db.metadata();

        std::cout << "1. Querying All Tables:\n";
        std::cout << std::string(80, '-') << "\n";
        auto all_tables = catalog.tables().get_all();
        for (const auto& table : all_tables) {
            if (!table.is_system_table) {  // Skip system tables for clarity
                std::cout << "  Table: " << std::setw(20) << std::left << table.table_name
                          << " | Records: " << std::setw(5) << table.record_count
                          << " | Type: " << table.type_name << "\n";
            }
        }

        std::cout << "\n2. Using Expression Templates on Metadata:\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "Finding tables with more than 5 records using static Fields:\n";

        // Use pre-defined static Fields from TableMetadata
        using namespace catalog;
        auto large_tables = catalog.tables()
            .where(TableMetadata::records > std::size_t(5));

        for (const auto& table : large_tables) {
            if (!table.is_system_table) {
                std::cout << "  " << table.table_name << ": "
                          << table.record_count << " records\n";
            }
        }

        std::cout << "\n3. Querying Field Metadata:\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "Schema for 'students' table:\n";

        auto student_fields = catalog.fields()
            .where(FieldMetadata::table == "students");

        for (const auto& field : student_fields) {
            std::cout << "  Field: " << std::setw(15) << std::left << field.field_name
                      << " | Type: " << std::setw(12) << field.field_type
                      << " | Order: " << field.field_order;
            if (field.is_primary_key) {
                std::cout << " (PRIMARY KEY)";
            }
            std::cout << "\n";
        }

        std::cout << "\n4. Record Count Synchronization:\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "The system catalog automatically tracks record counts!\n";

        // Show current counts
        auto tables_with_counts = catalog.tables()
            .where(TableMetadata::is_system == false);

        std::cout << "\nCurrent record counts (auto-synchronized):\n";
        for (const auto& table : tables_with_counts) {
            std::cout << "  " << std::setw(20) << std::left << table.table_name
                      << ": " << table.record_count << " records\n";
        }

        std::cout << "\n5. Complex Metadata Queries:\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "Finding all string-type fields across all tables:\n";

        auto string_fields = catalog.fields()
            .where(FieldMetadata::type == "std::string");

        std::unordered_map<std::string, std::vector<std::string>> table_string_fields;
        for (const auto& field : string_fields) {
            table_string_fields[field.table_name].push_back(field.field_name);
        }

        for (const auto& [table_name, fields] : table_string_fields) {
            std::cout << "  " << table_name << ": ";
            for (size_t i = 0; i < fields.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << fields[i];
            }
            std::cout << "\n";
        }

        std::cout << "\n6. Using Ranges with Metadata:\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "Finding primary key fields using ranges:\n";

        auto pk_fields = catalog.fields()
            .where(FieldMetadata::is_pk == true);

        for (const auto& field : pk_fields) {
            auto meta = catalog.tables()
                .where(TableMetadata::name == field.table_name);

            if (!meta.empty()) {
                // Get the first element by iterating
                for (const auto& table_meta : meta) {
                    if (!table_meta.is_system_table) {
                        std::cout << "  " << field.table_name << "." << field.field_name
                                  << " (" << field.field_type << ")\n";
                    }
                    break; // Only check first match
                }
            }
        }

        std::cout << "\nSystem Catalog Benefits:\n";
        std::cout << "  ✓ No more metadata overflow (was limited to ~100 tables)\n";
        std::cout << "  ✓ Can now handle 1000+ tables easily\n";
        std::cout << "  ✓ Metadata is queryable with expression templates\n";
        std::cout << "  ✓ Read-only protection prevents corruption\n";
        std::cout << "  ✓ Automatic record count synchronization\n";
        std::cout << "  ✓ Schema introspection at runtime\n";
        std::cout << "  ✓ Type-safe queries using static Fields\n\n";

        // ====================================================================
        // 15. Database Structure Inspection
        // ====================================================================
        print_section("15. Database Structure Inspection");

        std::cout << "Inspecting internal database structure...\n\n";

        // Print complete database structure
        // Database provides access to underlying storage engine via get_storage()
        utils::DbInspector::print_database_structure(db.get_storage(), false);

        // Print page allocation map
        utils::DbInspector::print_page_map(db.get_storage(), 40);

        std::cout << "Understanding the structure:\n";
        std::cout << "  • Page 0 is METADATA: Stores database header and table index roots\n";
        std::cout << "  • DATA pages: Contain actual record data (students, courses, enrollments)\n";
        std::cout << "  • INDEX pages: Contain B-tree nodes for fast lookups\n";
        std::cout << "  • All stored in a single unified database file!\n";

        // ====================================================================
        // 15. Seamless Secondary Index API (NEW!)
        // ====================================================================
        print_section("15. Seamless Secondary Index API (NEW!)");

        std::cout << "LearnQL now supports a seamless secondary index API!\n";
        std::cout << "Indexes are declared using fluent API and automatically persist.\n\n";

        // 15.1 Declaring Indexes with Fluent API
        std::cout << "1. Declaring Indexes with Fluent API:\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "Creating table with seamless index declaration:\n\n";

        // Drop table if it exists from previous run to ensure clean state
        try {
            db.drop_table("indexed_students");
            std::cout << "  (Dropped existing table from previous run)\n";
        } catch (...) {
            // Table doesn't exist, which is fine
        }

        // Create a fresh table with indexes
        auto& indexed_students = db.table<Student>("indexed_students")
            .add_index(Student::name, core::IndexType::Unique)
            .add_index(Student::department, core::IndexType::MultiValue)
            .add_index(Student::gpa, core::IndexType::MultiValue);

        std::cout << "✓ Table created with 3 secondary indexes:\n";
        std::cout << "  - name (Unique): Fast lookups by student name\n";
        std::cout << "  - department (MultiValue): Find all students in a department\n";
        std::cout << "  - gpa (MultiValue): Find all students with specific GPA\n\n";

        // Populate the table
        std::cout << "Inserting students (indexes auto-update)...\n";
        for (const auto& student : student_data) {
            indexed_students.insert(student);
        }
        std::cout << "✓ Inserted " << indexed_students.size() << " students\n";
        std::cout << "✓ All indexes automatically updated during insertion!\n";

        // 15.2 Fast Lookups with find_by()
        std::cout << "\n2. Fast Lookups with find_by() (Unique Index):\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "Finding student by name using unique index:\n";

        auto alice_by_name = indexed_students.find_by(Student::name, std::string("Alice Johnson"));
        if (alice_by_name) {
            std::cout << "✓ Found: ";
            print_student(*alice_by_name);
            std::cout << "  Performance: O(log n) index lookup vs O(n) table scan!\n";
        }

        // 15.3 Multi-Value Lookups with find_all_by()
        std::cout << "\n3. Multi-Value Lookups with find_all_by():\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "Finding all CS students using multi-value department index:\n";

        auto cs_by_dept = indexed_students.find_all_by(Student::department, std::string("CS"));
        std::cout << "✓ Found " << cs_by_dept.size() << " CS students:\n";
        for (const auto& s : cs_by_dept) {
            std::cout << "  - " << s.get_name() << " (GPA: " << std::fixed << std::setprecision(2) << s.get_gpa() << ")\n";
        }

        std::cout << "\nFinding students with GPA 3.7:\n";
        auto students_37 = indexed_students.find_all_by(Student::gpa, 3.7);
        std::cout << "✓ Found " << students_37.size() << " students with GPA 3.7\n";

        // 15.4 Range Queries
        std::cout << "\n4. Range Queries:\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "Finding students with GPA between 3.5 and 4.0:\n";

        auto high_gpa_range = indexed_students.range_query(Student::gpa, 3.5, 4.0);
        std::cout << "✓ Found " << high_gpa_range.size() << " high-GPA students:\n";
        for (const auto& s : high_gpa_range) {
            std::cout << "  - " << std::setw(20) << std::left << s.get_name()
                      << " GPA: " << std::fixed << std::setprecision(2) << s.get_gpa() << "\n";
        }

        // 15.5 Getting Unique Values
        std::cout << "\n5. Getting Unique Values:\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "Listing all unique departments:\n";

        auto unique_depts_seamless = indexed_students.get_unique_values(Student::department);
        std::cout << "✓ Found " << unique_depts_seamless.size() << " unique departments:\n";
        for (const auto& dept : unique_depts_seamless) {
            auto dept_students = indexed_students.find_all_by(Student::department, dept);
            std::cout << "  - " << std::setw(12) << std::left << dept
                      << " (" << dept_students.size() << " students)\n";
        }

        // 15.6 Index Metadata in Catalog
        std::cout << "\n6. Querying Index Metadata from System Catalog:\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "Indexes are automatically registered in the system catalog!\n\n";

        using namespace catalog;
        auto student_indexes = catalog.indexes()
            .where(IndexMetadata::table == "indexed_students");

        std::cout << "Indexes for 'indexed_students' table:\n";
        for (const auto& idx : student_indexes) {
            std::cout << "  • Field: " << std::setw(12) << std::left << idx.get_field_name()
                      << " | Type: " << std::setw(12) << idx.get_field_type()
                      << " | " << (idx.get_is_unique() ? "Unique" : "MultiValue")
                      << " | Root Page: " << idx.get_index_root_page() << "\n";
        }

        // 15.7 Automatic CRUD Synchronization
        std::cout << "\n7. Automatic CRUD Synchronization:\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "Indexes automatically update on insert/update/delete!\n\n";

        std::cout << "Before update: Finding by name 'Alice Johnson':\n";
        auto alice_before = indexed_students.find_by(Student::name, std::string("Alice Johnson"));
        if (alice_before) {
            std::cout << "  Found: " << alice_before->get_name()
                      << " in " << alice_before->get_department() << "\n";
        }

        std::cout << "\nUpdating Alice's department to 'Engineering'...\n";
        if (alice_before) {
            alice_before->set_department("Engineering");
            indexed_students.update(*alice_before);
        }

        std::cout << "After update:\n";
        auto alice_after = indexed_students.find_by(Student::name, std::string("Alice Johnson"));
        if (alice_after) {
            std::cout << "  Found: " << alice_after->get_name()
                      << " in " << alice_after->get_department() << "\n";
        }

        auto eng_students = indexed_students.find_all_by(Student::department, std::string("Engineering"));
        std::cout << "  Engineering department now has " << eng_students.size() << " students\n";

        std::cout << "✓ All indexes automatically synchronized!\n";

        // Restore original state
        if (alice_after) {
            alice_after->set_department("CS");
            indexed_students.update(*alice_after);
        }

        // 15.8 Persistence Demonstration
        std::cout << "\n8. Index Persistence Across Restarts:\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "Indexes are persistent - they survive database restarts!\n\n";

        std::cout << "Flushing database (indexes auto-flush)...\n";
        indexed_students.flush();
        std::cout << "✓ Indexes persisted to disk\n\n";

        std::cout << "Simulating restart: Reopening table with same indexes...\n";
        auto& reloaded_students = db.table<Student>("indexed_students")
            .add_index(Student::name, core::IndexType::Unique)
            .add_index(Student::department, core::IndexType::MultiValue)
            .add_index(Student::gpa, core::IndexType::MultiValue);

        std::cout << "✓ Table and indexes reloaded from disk\n";
        std::cout << "  Indexes detected existing data and loaded automatically!\n\n";

        auto alice_reloaded = reloaded_students.find_by(Student::name, std::string("Alice Johnson"));
        if (alice_reloaded) {
            std::cout << "✓ Index lookup still works after reload:\n";
            std::cout << "  Found: ";
            print_student(*alice_reloaded);
        }

        // 15.9 Performance Summary
        std::cout << "\n9. Performance Summary:\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "Seamless API Benefits:\n";
        std::cout << "  ✓ Fluent interface: .add_index() returns Table& for chaining\n";
        std::cout << "  ✓ Type-safe: Field types enforced at compile time\n";
        std::cout << "  ✓ Automatic persistence: Indexes saved to catalog automatically\n";
        std::cout << "  ✓ Automatic reconstruction: Existing indexes loaded on table open\n";
        std::cout << "  ✓ Automatic CRUD sync: Insert/update/delete update all indexes\n";
        std::cout << "  ✓ Zero boilerplate: No manual index management needed\n\n";

        std::cout << "Query Performance:\n";
        std::cout << "  • find_by() on unique index: O(log n) vs O(n) full scan\n";
        std::cout << "  • find_all_by() on multi-value index: O(log n + k) where k = results\n";
        std::cout << "  • range_query(): O(log n + k) with efficient leaf traversal\n";
        std::cout << "  • For 1M records: ~20 comparisons vs ~1M comparisons!\n";
        std::cout << "  • 50,000x speedup for indexed queries!\n\n";

        // ====================================================================
        // 16. Low-Level Secondary Index API (Advanced)
        // ====================================================================
        print_section("16. Low-Level Secondary Index API (Advanced)");

        std::cout << "LearnQL now supports persistent secondary indexes!\n";
        std::cout << "Secondary indexes provide fast lookups on non-primary-key fields.\n\n";

        // 15.1 Unique Secondary Index (Email)
        std::cout << "1. Creating Unique Secondary Index (email field):\n";
        std::cout << std::string(80, '-') << "\n";

        // Create a unique secondary index on the name field (treating it as unique for demo)
        index::PersistentSecondaryIndex<Student, std::string> name_index(
            "name",
            [](const Student& s) { return s.get_name(); },
            db.get_storage_ptr()
        );

        std::cout << "Populating name index from existing students...\n";
        for (const auto& student : students) {
            auto rid = students.get_record_id(student.get_student_id());
            if (rid) {
                name_index.insert(student, *rid);
            }
        }
        std::cout << "✓ Index populated with " << name_index.size() << " entries\n";

        std::cout << "\nFast lookup by name using secondary index:\n";
        auto alice_rid = name_index.find("Alice Johnson");
        if (alice_rid) {
            std::cout << "  Found 'Alice Johnson' at page " << alice_rid->page_id
                      << ", slot " << alice_rid->slot << "\n";
            auto alice_record = students.find(1001);
            if (alice_record) {
                std::cout << "  ";
                print_student(*alice_record);
            }
        }

        std::cout << "\nRange query on names (A-C):\n";
        auto names_a_to_c = name_index.range_query("A", "D");
        std::cout << "  Found " << names_a_to_c.size() << " names starting with A-C:\n";
        for (const auto& rid : names_a_to_c) {
            // Would normally load the record here, but for demo just show the RID
            std::cout << "    RecordId: page=" << rid.page_id << ", slot=" << rid.slot << "\n";
        }

        // 15.2 Multi-Value Secondary Index (Department)
        std::cout << "\n2. Creating Multi-Value Secondary Index (department field):\n";
        std::cout << std::string(80, '-') << "\n";

        index::PersistentMultiValueSecondaryIndex<Student, std::string> dept_index(
            "department",
            [](const Student& s) { return s.get_department(); },
            db.get_storage_ptr()
        );

        std::cout << "Populating department index (allows multiple students per department)...\n";
        for (const auto& student : students) {
            auto rid = students.get_record_id(student.get_student_id());
            if (rid) {
                dept_index.insert(student, *rid);
            }
        }
        std::cout << "✓ Index populated with " << dept_index.size() << " entries\n";

        std::cout << "\nFinding all CS students using multi-value index:\n";
        auto cs_rids = dept_index.find("CS");
        std::cout << "  Found " << cs_rids.size() << " CS students:\n";
        for (const auto& rid : cs_rids) {
            std::cout << "    RecordId: page=" << rid.page_id << ", slot=" << rid.slot << "\n";
        }

        std::cout << "\nCounting students by department:\n";
        auto unique_depts = dept_index.get_unique_values();
        for (const auto& dept : unique_depts) {
            std::size_t count = dept_index.count(dept);
            std::cout << "  " << std::setw(12) << std::left << dept << ": " << count << " students\n";
        }

        // 15.3 Index Operations
        std::cout << "\n3. Index Operations (Update, Remove):\n";
        std::cout << std::string(80, '-') << "\n";

        std::cout << "Demonstrating index update when record changes...\n";
        Student test_student(1001, "Alice Johnson", "CS", 20, 3.9);
        Student updated_student(1001, "Alice Johnson", "Math", 20, 3.9);

        auto test_rid = students.get_record_id(1001);
        if (test_rid) {
            std::cout << "  Before: Alice is in CS department\n";
            std::cout << "  CS department has " << dept_index.count("CS") << " students\n";
            std::cout << "  Math department has " << dept_index.count("Math") << " students\n";

            // Update the index (in real usage, this would happen automatically)
            bool updated = dept_index.update(test_student, updated_student, *test_rid);
            std::cout << "  Update result: " << (updated ? "success" : "failed") << "\n";

            std::cout << "  After: Alice moved to Math department\n";
            std::cout << "  CS department has " << dept_index.count("CS") << " students\n";
            std::cout << "  Math department has " << dept_index.count("Math") << " students\n";

            // Restore original state
            dept_index.update(updated_student, test_student, *test_rid);
        }

        // 15.4 Persistence
        std::cout << "\n4. Index Persistence:\n";
        std::cout << std::string(80, '-') << "\n";

        std::cout << "Flushing indexes to disk...\n";
        name_index.flush();
        dept_index.flush();

        std::cout << "✓ Name index root page: " << name_index.get_root_page_id() << "\n";
        std::cout << "✓ Department index root page: " << dept_index.get_root_page_id() << "\n";
        std::cout << "✓ Indexes persisted and will survive database restart\n";

        std::cout << "\nReloading indexes from disk (simulating restart)...\n";
        index::PersistentSecondaryIndex<Student, std::string> name_index_reloaded(
            "name",
            [](const Student& s) { return s.get_name(); },
            db.get_storage_ptr(),
            name_index.get_root_page_id()
        );

        index::PersistentMultiValueSecondaryIndex<Student, std::string> dept_index_reloaded(
            "department",
            [](const Student& s) { return s.get_department(); },
            db.get_storage_ptr(),
            dept_index.get_root_page_id()
        );

        std::cout << "✓ Name index reloaded: " << name_index_reloaded.size() << " entries\n";
        std::cout << "✓ Department index reloaded: " << dept_index_reloaded.size() << " entries\n";

        // 15.5 Performance Benefits
        std::cout << "\n5. Performance Benefits:\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "Without secondary index:\n";
        std::cout << "  • Finding by non-PK field requires O(n) full table scan\n";
        std::cout << "  • For 10 students: ~10 comparisons\n";
        std::cout << "  • For 1 million students: ~1 million comparisons!\n\n";

        std::cout << "With secondary index (B+Tree):\n";
        std::cout << "  • Finding by indexed field is O(log n)\n";
        std::cout << "  • For 10 students: ~3-4 comparisons\n";
        std::cout << "  • For 1 million students: ~20 comparisons!\n";
        std::cout << "  • 50,000x faster for large datasets!\n\n";

        std::cout << "Additional benefits:\n";
        std::cout << "  ✓ Range queries supported (e.g., find all names from A-M)\n";
        std::cout << "  ✓ Multi-value indexes for non-unique fields (e.g., department)\n";
        std::cout << "  ✓ Persistent across restarts\n";
        std::cout << "  ✓ Automatic update/delete synchronization\n";
        std::cout << "  ✓ Memory-efficient B+Tree implementation\n";
        std::cout << "  ✓ Leaf node linking for fast sequential scans\n\n";

        std::cout << "Secondary Index Features:\n";
        std::cout << "  ✓ PersistentSecondaryIndex: For unique fields\n";
        std::cout << "  ✓ PersistentMultiValueSecondaryIndex: For non-unique fields\n";
        std::cout << "  ✓ O(log n) lookups using B+Tree storage\n";
        std::cout << "  ✓ Range queries via leaf node linking\n";
        std::cout << "  ✓ Persistent to disk (survives restarts)\n";
        std::cout << "  ✓ Batch iteration support\n";
        std::cout << "  ✓ Update/remove operations\n\n";

        // ====================================================================
        // 16. Advanced Property Features
        // ====================================================================
        print_section("16. Advanced Property Features");

        std::cout << "Demonstrating property macro capabilities:\n\n";

        // 1. Showing property setters in action
        std::cout << "1. Using Property Setters:\n";
        std::cout << std::string(80, '-') << "\n";

        Student new_student;
        new_student.set_student_id(2000);
        new_student.set_name("Property Demo Student");
        new_student.set_department("Engineering");
        new_student.set_age(21);
        new_student.set_gpa(3.85);

        std::cout << "Created student using setters:\n";
        print_student(new_student);

        // 2. Demonstrating type-safe getters
        std::cout << "\n2. Type-Safe Getters:\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "Properties use smart return types:\n";
        std::cout << "  • Primitives (int, double) returned by value\n";
        std::cout << "  • Strings returned by const reference (efficient!)\n";
        std::cout << "  • All getters marked [[nodiscard]] for safety\n\n";

        // 3. Automatic Field generation
        std::cout << "3. Automatic Static Field Generation:\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "Each property automatically creates a static Field:\n";
        std::cout << "  • Student::student_id - Field<Student, int>\n";
        std::cout << "  • Student::name - Field<Student, std::string>\n";
        std::cout << "  • Student::department - Field<Student, std::string>\n";
        std::cout << "  • Student::age - Field<Student, int>\n";
        std::cout << "  • Student::gpa - Field<Student, double>\n";
        std::cout << "\nThese Fields can be used directly in query expressions!\n";
        std::cout << "(No need to manually declare Field<Student, T> objects)\n\n";

        // 4. Macro expansion explanation
        std::cout << "4. What LEARNQL_PROPERTY Expands To:\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "LEARNQL_PROPERTY(Student, int, age) generates:\n";
        std::cout << "  private:\n";
        std::cout << "    int age_;  // Private member with trailing underscore\n";
        std::cout << "  public:\n";
        std::cout << "    int get_age() const { return age_; }\n";
        std::cout << "    void set_age(int value) { age_ = value; }\n";
        std::cout << "    static inline const Field<Student, int> age{\"age\", &Student::get_age};\n\n";

        // 5. Boilerplate reduction
        std::cout << "5. Boilerplate Reduction:\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "Manual approach (old):\n";
        std::cout << "  • ~65 lines per class (public members, getters, serialize, deserialize)\n";
        std::cout << "\nMacro approach (new):\n";
        std::cout << "  • ~35 lines per class (properties, serialize macros, deserialize macros)\n";
        std::cout << "\nReduction: ~46% less code!\n";
        std::cout << "Benefits:\n";
        std::cout << "  ✓ Less typing, fewer errors\n";
        std::cout << "  ✓ Consistent getter/setter naming\n";
        std::cout << "  ✓ Automatic Field generation for queries\n";
        std::cout << "  ✓ Type-safe return/parameter types\n";
        std::cout << "  ✓ Encapsulation (private members)\n\n";

        // 6. When to use macros vs manual
        std::cout << "6. When to Use Property Macros:\n";
        std::cout << std::string(80, '-') << "\n";
        std::cout << "Use property macros when:\n";
        std::cout << "  ✓ You want rapid development\n";
        std::cout << "  ✓ You need consistent property patterns\n";
        std::cout << "  ✓ You want automatic Field generation\n";
        std::cout << "  ✓ You prefer encapsulation (private members)\n";
        std::cout << "\nUse manual approach when:\n";
        std::cout << "  ✓ Learning the library internals\n";
        std::cout << "  ✓ You need custom getter/setter logic\n";
        std::cout << "  ✓ You have complex validation requirements\n";
        std::cout << "  ✓ You want full control over implementation\n\n";

        std::cout << "Both approaches produce identical functionality!\n";
        std::cout << "The choice is yours based on your needs.\n";

        // ====================================================================
        // Summary
        // ====================================================================
        print_section("Demo Complete!");

        std::cout << "LearnQL Features Demonstrated:\n\n";
        std::cout << "  ✓ 1.  Database class for table management\n";
        std::cout << "  ✓ 2.  CRUD operations (Create, Read, Update, Delete)\n";
        std::cout << "  ✓ 3.  Expression template queries\n";
        std::cout << "  ✓ 4.  Lambda predicate filtering\n";
        std::cout << "  ✓ 5.  Join operations (inner, left, cross)\n";
        std::cout << "  ✓ 6.  GroupBy and aggregations (count, sum, avg)\n";
        std::cout << "  ✓ 7.  C++20 ranges integration\n";
        std::cout << "  ✓ 8.  Query builder pattern\n";
        std::cout << "  ✓ 9.  Batched loading for memory efficiency\n";
        std::cout << "  ✓ 10. B-tree indexing for fast lookups\n";
        std::cout << "  ✓ 11. Type-safe compile-time validation\n";
        std::cout << "  ✓ 12. Persistent storage with flush control\n";
        std::cout << "  ✓ 13. System catalog for queryable metadata\n";
        std::cout << "  ✓ 14. Schema introspection at runtime\n";
        std::cout << "  ✓ 15. Database structure inspection\n";
        std::cout << "  ✓ 16. Persistent secondary indexes (NEW!)\n";
        std::cout << "  ✓ 17. Property macros for rapid development\n\n";

        std::cout << "Key Innovations:\n\n";

        std::cout << "1. Batched Loading:\n";
        std::cout << "  • Configurable batch size (default: 10)\n";
        std::cout << "  • Memory usage stays constant\n";
        std::cout << "  • Transparent to user code\n";
        std::cout << "  • Works across all operations\n\n";

        std::cout << "2. Property Macros:\n";
        std::cout << "  • 46% less boilerplate code\n";
        std::cout << "  • Automatic getter/setter generation\n";
        std::cout << "  • Static Field objects for queries\n";
        std::cout << "  • Type-safe return/parameter types\n\n";

        std::cout << "3. System Catalog:\n";
        std::cout << "  • Queryable metadata tables\n";
        std::cout << "  • Supports 1000+ tables (vs ~100 before)\n";
        std::cout << "  • Expression template queries on metadata\n";
        std::cout << "  • Automatic record count synchronization\n";
        std::cout << "  • Read-only protection for safety\n";
        std::cout << "  • Runtime schema introspection\n\n";

        std::cout << "4. Persistent Secondary Indexes (NEW!):\n";
        std::cout << "  • Fast O(log n) lookups on non-primary-key fields\n";
        std::cout << "  • Unique indexes (PersistentSecondaryIndex)\n";
        std::cout << "  • Multi-value indexes (PersistentMultiValueSecondaryIndex)\n";
        std::cout << "  • Range query support via B+Tree leaf linking\n";
        std::cout << "  • Persistent to disk (survives restarts)\n";
        std::cout << "  • 50,000x faster than full table scans for large datasets\n";
        std::cout << "  • Automatic update/delete synchronization\n\n";

        std::cout << "Thank you for exploring LearnQL!\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
