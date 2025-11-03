# Contributing to LearnQL

Thank you for your interest in contributing to LearnQL! This document provides guidelines and instructions for contributing to the project.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [How to Contribute](#how-to-contribute)
- [Coding Standards](#coding-standards)
- [Testing Guidelines](#testing-guidelines)
- [Documentation](#documentation)
- [Pull Request Process](#pull-request-process)
- [Issue Guidelines](#issue-guidelines)

## Code of Conduct

By participating in this project, you agree to maintain a respectful and inclusive environment:

- Be respectful of differing viewpoints and experiences
- Accept constructive criticism gracefully
- Focus on what is best for the community
- Show empathy towards other community members

## Getting Started

### Prerequisites

- C++20 compliant compiler:
  - GCC 10 or higher
  - Clang 12 or higher
  - MSVC 19.29 or higher
- CMake 3.23 or higher
- Git for version control

### Fork and Clone

1. Fork the repository on GitHub
2. Clone your fork locally:
   ```bash
   git clone https://github.com/tdelphi1981/LearnQL.git
   cd LearnQL
   ```
3. Add the upstream repository:
   ```bash
   git remote add upstream https://github.com/tdelphi1981/LearnQL.git
   ```

## Development Setup

### Building the Project

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build
cmake --build .

# Run the example
./LearnQL
```

### IDE Setup

#### CLion
Open the project directory in CLion - it will automatically detect the CMake configuration.

#### Visual Studio Code
Install the C/C++ and CMake Tools extensions, then open the project folder.

#### Visual Studio
Use "Open Folder" and VS will detect the CMake configuration.

## How to Contribute

### Types of Contributions

We welcome various types of contributions:

1. **Bug Fixes** - Fix issues in existing code
2. **New Features** - Add new functionality
3. **Documentation** - Improve or add documentation
4. **Examples** - Create example code demonstrating features
5. **Performance Improvements** - Optimize existing code
6. **Tests** - Add or improve test coverage
7. **Code Quality** - Refactoring and code cleanup

### Before You Start

1. **Check existing issues** - See if your idea is already being discussed
2. **Open an issue first** - For major changes, discuss your approach before implementing
3. **Keep changes focused** - One feature/fix per pull request

## Coding Standards

### Style Guidelines

LearnQL follows modern C++ best practices:

#### General Principles

- Use C++20 features where appropriate
- Follow RAII principles
- Prefer `const` correctness
- Use `auto` when type is obvious from context
- Avoid raw pointers; prefer smart pointers or references

#### Naming Conventions

```cpp
// Classes: PascalCase
class StudentRecord { };

// Functions/Methods: camelCase
void calculateAverage() { }

// Variables: snake_case
int student_count = 0;

// Constants: UPPER_SNAKE_CASE
constexpr int MAX_PAGE_SIZE = 4096;

// Private members: trailing underscore
class Example {
    int value_;
    std::string name_;
};

// Namespaces: lowercase
namespace learnql::core { }

// Template parameters: PascalCase
template<typename RecordType>
class Table { };
```

#### File Organization

```cpp
// 1. Header guard or #pragma once
#ifndef LEARNQL_EXAMPLE_HPP
#define LEARNQL_EXAMPLE_HPP

// 2. System includes
#include <string>
#include <vector>

// 3. External library includes
// (none for LearnQL as it's standalone)

// 4. Project includes
#include "learnql/core/Table.hpp"

// 5. Namespace
namespace learnql::example {

// 6. Class/function definitions

} // namespace learnql::example

#endif // LEARNQL_EXAMPLE_HPP
```

#### Code Formatting

- Indentation: 4 spaces (no tabs)
- Line length: Maximum 100 characters (prefer 80)
- Braces: Opening brace on same line
  ```cpp
  void function() {
      // code
  }
  ```
- Pointer/reference alignment: Attach to type
  ```cpp
  int* ptr;      // Preferred
  std::string& ref;  // Preferred
  ```

### Header-Only Library Guidelines

Since LearnQL is header-only:

- All implementations must be in headers
- Use `inline` for non-template functions defined in headers
- Templates don't need `inline` (implicitly inline)
- Be mindful of compile times
- Minimize includes in public headers

### Documentation

#### Doxygen Comments

Document all public APIs:

```cpp
/**
 * @brief Brief description of the function
 *
 * Detailed description of what the function does,
 * its behavior, and any important notes.
 *
 * @param name Parameter description
 * @param count Another parameter
 * @return Description of return value
 *
 * @throws std::runtime_error If something goes wrong
 *
 * @code
 * // Usage example
 * auto result = myFunction("test", 42);
 * @endcode
 */
int myFunction(const std::string& name, int count);
```

#### Inline Comments

Use comments to explain **why**, not **what**:

```cpp
// Good: Explains why
// Use binary search as the index is sorted
auto it = std::lower_bound(index.begin(), index.end(), key);

// Bad: Explains what (obvious from code)
// Increment counter
counter++;
```

### Error Handling

- Use exceptions for exceptional conditions
- Provide clear error messages
- Document exceptions in function comments
- Use standard exception types when appropriate

```cpp
if (page_id >= total_pages_) {
    throw std::out_of_range(
        "Page ID " + std::to_string(page_id) +
        " exceeds total pages " + std::to_string(total_pages_)
    );
}
```

## Testing Guidelines

### Writing Tests

While LearnQL doesn't currently have a formal test suite, contributors should:

1. **Test your changes** - Ensure new code works as expected
2. **Test edge cases** - Consider boundary conditions
3. **Test error paths** - Verify error handling works
4. **Add examples** - Demonstrate usage of new features in main.cpp or separate examples

### Manual Testing Checklist

Before submitting a PR:

- [ ] Code compiles without warnings on GCC, Clang, and MSVC
- [ ] All features work as documented
- [ ] Memory leaks checked (use valgrind or sanitizers)
- [ ] Edge cases tested
- [ ] Documentation is accurate

### Building with Sanitizers

```bash
# Address Sanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address -g" ..
cmake --build .

# Undefined Behavior Sanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=undefined -g" ..
cmake --build .
```

## Documentation

### Documentation Standards

- All public APIs must be documented
- Use Doxygen format for API documentation
- Include code examples for complex features
- Keep README.md updated with new features
- Update inline comments when refactoring

### Documentation Locations

- **API docs**: Inline in header files
- **Usage guides**: README.md
- **Examples**: main.cpp or separate example files
- **Architecture**: High-level comments in major headers

## Pull Request Process

### Before Submitting

1. **Update your fork**:
   ```bash
   git fetch upstream
   git rebase upstream/main
   ```

2. **Create a feature branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make your changes**:
   - Follow coding standards
   - Add documentation
   - Test thoroughly

4. **Commit your changes**:
   ```bash
   git add .
   git commit -m "Brief description of changes"
   ```

### Commit Message Guidelines

Follow these conventions:

```
type: Brief summary (50 chars or less)

More detailed explanation if needed. Wrap at 72 characters.
Explain the problem this commit solves and how.

- Bullet points are okay
- Use present tense: "Add feature" not "Added feature"
- Reference issues: "Fixes #123"
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Formatting, no code change
- `refactor`: Code restructuring
- `perf`: Performance improvement
- `test`: Adding tests
- `chore`: Maintenance tasks

### Submitting the PR

1. **Push to your fork**:
   ```bash
   git push origin feature/your-feature-name
   ```

2. **Open a Pull Request** on GitHub

3. **Fill out the PR template** with:
   - Description of changes
   - Related issues
   - Testing performed
   - Breaking changes (if any)

### PR Review Process

- Maintainers will review your PR
- Address any requested changes
- Once approved, your PR will be merged

### After Merge

Delete your feature branch:
```bash
git branch -d feature/your-feature-name
git push origin --delete feature/your-feature-name
```

## Issue Guidelines

### Reporting Bugs

Include:
- **Description**: Clear description of the bug
- **Steps to reproduce**: Minimal code example
- **Expected behavior**: What should happen
- **Actual behavior**: What actually happens
- **Environment**: OS, compiler, version
- **Error messages**: Full error output

Example:
```markdown
## Bug Description
Query with nested logical operators crashes

## Steps to Reproduce
\`\`\`cpp
auto results = table.query()
    .where((Field1 == "A" && Field2 == "B") || (Field3 == "C" && Field4 == "D"))
    .collect();
\`\`\`

## Expected
Should return filtered results

## Actual
Segmentation fault

## Environment
- OS: Ubuntu 22.04
- Compiler: GCC 11.2
- CMake: 3.24
```

### Suggesting Features

Include:
- **Use case**: Why is this feature needed?
- **Proposed solution**: How should it work?
- **Alternatives**: Other approaches considered
- **Example usage**: Code showing how it would be used

## Questions?

If you have questions:
- Open an issue with the "question" label
- Check existing documentation
- Review the example code in main.cpp

## License

By contributing, you agree that your contributions will be licensed under the Academic Free License 3.0.

---

Thank you for contributing to LearnQL!
