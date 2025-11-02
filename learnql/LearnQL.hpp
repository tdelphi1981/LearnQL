#ifndef LEARNQL_HPP
#define LEARNQL_HPP

/**
 * @file LearnQL.hpp
 * @brief Master include file for the LearnQL library
 *
 * This header includes all LearnQL components. Include this file to access
 * the entire LearnQL API with a single include directive.
 *
 * Usage:
 * @code
 * #include <learnql/LearnQL.hpp>
 *
 * using namespace learnql;
 *
 * // Using Database class (recommended)
 * core::Database db("mydb.db");
 * auto& students = db.table<Student>("students");
 *
 * // Or using direct Table construction
 * auto storage = std::make_shared<storage::StorageEngine>("mydb.db");
 * core::Table<Student> students(storage, "students");
 * @endcode
 *
 * LearnQL is a modern C++20 database library featuring:
 * - Type-safe table operations with concepts
 * - Expression template query DSL (SQL-like syntax)
 * - Property macros for automatic code generation (70% less boilerplate)
 * - Compile-time reflection system
 * - Queryable system catalog for metadata introspection
 * - Seamless secondary index API with fluent interface and auto-persistence
 * - B-tree indexing with batched loading
 * - Read-only tables for metadata protection
 * - C++20 ranges integration
 * - Coroutine support for async queries
 * - Page-based persistent storage
 *
 * For modular includes, you can instead include individual headers:
 * - #include "learnql/core/Table.hpp"
 * - #include "learnql/query/Query.hpp"
 * - #include "learnql/meta/Property.hpp"
 * - #include "learnql/catalog/SystemCatalog.hpp"
 * - etc.
 */

// ============================================================================
// Core Components
// ============================================================================

#include "core/RecordId.hpp"
#include "core/Table.hpp"
#include "core/ReadOnlyTable.hpp"
#include "core/Database.hpp"

// ============================================================================
// Storage Layer
// ============================================================================

#include "storage/Page.hpp"
#include "storage/StorageEngine.hpp"

// ============================================================================
// Serialization
// ============================================================================

#include "serialization/BinaryWriter.hpp"
#include "serialization/BinaryReader.hpp"

// ============================================================================
// Concepts
// ============================================================================

#include "concepts/Serializable.hpp"
#include "concepts/Queryable.hpp"

// ============================================================================
// Meta Programming
// ============================================================================

#include "meta/TypeInfo.hpp"
#include "meta/Property.hpp"

// ============================================================================
// Reflection System
// ============================================================================

#include "reflection/FieldInfo.hpp"
#include "reflection/FieldExtractor.hpp"

// ============================================================================
// System Catalog
// ============================================================================

#include "catalog/TableMetadata.hpp"
#include "catalog/FieldMetadata.hpp"
#include "catalog/IndexMetadata.hpp"
#include "catalog/SystemCatalog.hpp"

// ============================================================================
// Index
// ============================================================================

#include "index/BatchIterator.hpp"
#include "index/PersistentBTreeIndex.hpp"
#include "index/PersistentSecondaryIndex.hpp"
#include "index/PersistentMultiValueSecondaryIndex.hpp"

// ============================================================================
// Query System - Expressions (must come before Field, Query, etc.)
// ============================================================================

#include "query/expressions/Expr.hpp"
#include "query/expressions/ConstExpr.hpp"
#include "query/expressions/FieldExpr.hpp"
#include "query/expressions/BinaryExpr.hpp"
#include "query/expressions/LogicalExpr.hpp"

// ============================================================================
// Query System - Core
// ============================================================================

#include "query/Field.hpp"
#include "query/Query.hpp"
#include "query/Join.hpp"
#include "query/GroupBy.hpp"

// ============================================================================
// Ranges
// ============================================================================

#include "ranges/ProxyVector.hpp"
#include "ranges/QueryView.hpp"
#include "ranges/Adaptors.hpp"

// ============================================================================
// Coroutines (Optional - requires C++20 coroutine support)
// ============================================================================

#include "coroutines/Generator.hpp"
#include "coroutines/AsyncQuery.hpp"

// ============================================================================
// Debug & Profiling Utilities
// ============================================================================

#include "debug/Statistics.hpp"
#include "debug/Profiler.hpp"
#include "debug/ExecutionPlan.hpp"
#include "debug/DebugUtils.hpp"

// ============================================================================
// Utilities
// ============================================================================

#include "utils/DbInspector.hpp"

/**
 * @namespace learnql
 * @brief Root namespace for all LearnQL components
 *
 * Sub-namespaces:
 * - learnql::core - Core database and table classes
 * - learnql::storage - Page-based storage engine
 * - learnql::serialization - Binary serialization utilities
 * - learnql::concepts - C++20 concepts for type validation
 * - learnql::meta - Type metadata and property macros
 * - learnql::reflection - Compile-time reflection system
 * - learnql::catalog - System catalog for queryable metadata
 * - learnql::index - B-tree and secondary indexes
 * - learnql::query - Query DSL and expression templates
 * - learnql::ranges - C++20 ranges integration
 * - learnql::coroutines - Coroutine-based async queries
 * - learnql::debug - Debugging and profiling utilities
 * - learnql::utils - Utility functions and helpers
 */

#endif // LEARNQL_HPP
