# LearnQL Documentation

This directory contains the complete documentation for LearnQL, built with Sphinx and published on ReadTheDocs.

## Documentation Structure

```
docs/
├── getting-started/     # Installation, quick start, core concepts, best practices
├── tutorials/           # 7 progressive tutorials from beginner to advanced
├── api/                 # API reference for all modules
├── architecture/        # Deep-dive into internal architecture
├── resources/           # C++20 glossary, FAQ, examples, contributing guide
├── conf.py              # Sphinx configuration
├── Doxyfile             # Doxygen configuration
└── requirements.txt     # Python dependencies
```

## Building Documentation Locally

### Prerequisites

- Python 3.11+
- Doxygen
- Graphviz (optional, for diagrams)

### Installation

1. **Install Python dependencies:**

   ```bash
   pip3 install -r requirements.txt
   ```

2. **Install Doxygen:**

   On macOS:
   ```bash
   brew install doxygen
   ```

   On Ubuntu/Debian:
   ```bash
   sudo apt-get install doxygen
   ```

   On Windows:
   Download from https://www.doxygen.nl/download.html

3. **Install Graphviz (optional):**

   On macOS:
   ```bash
   brew install graphviz
   ```

   On Ubuntu/Debian:
   ```bash
   sudo apt-get install graphviz
   ```

### Building

1. **Generate Doxygen XML:**

   ```bash
   cd docs
   doxygen Doxyfile
   ```

   This will create XML files in `docs/doxyxml/` for API documentation.

2. **Build HTML documentation:**

   ```bash
   sphinx-build -b html . _build/html
   ```

   Or use the make command (if available):
   ```bash
   make html
   ```

3. **View the documentation:**

   Open `_build/html/index.html` in your web browser:

   ```bash
   open _build/html/index.html  # macOS
   xdg-open _build/html/index.html  # Linux
   start _build/html/index.html  # Windows
   ```

### Building PDF (Optional)

To build PDF documentation:

```bash
sphinx-build -b latex . _build/latex
cd _build/latex
make
```

## ReadTheDocs Integration

This documentation is configured to automatically build on ReadTheDocs when pushed to the repository.

### Configuration Files

- `.readthedocs.yaml` - ReadTheDocs build configuration
- `docs/conf.py` - Sphinx configuration
- `docs/Doxyfile` - Doxygen configuration for C++ API extraction
- `docs/requirements.txt` - Python package dependencies

### Automatic Builds

ReadTheDocs will:
1. Install Python dependencies from `requirements.txt`
2. Install Doxygen and Graphviz
3. Run Doxygen to generate XML from C++ code
4. Build Sphinx documentation
5. Generate HTML, PDF, and ePub formats
6. Deploy to https://learnql.readthedocs.io/

## Documentation Content

### Getting Started
- **Installation**: Compiler requirements and setup
- **Quick Start**: 30-minute introduction
- **Core Concepts**: Fundamental concepts for beginners
- **Property Macros**: In-depth guide to the property system
- **Best Practices**: Patterns and anti-patterns

### Tutorials (7 Progressive Tutorials)
1. **First Database**: Build a library management system
2. **CRUD Operations**: Complete create, read, update, delete
3. **Query DSL**: Master the query expression system
4. **Joins & Relationships**: Working with related data
5. **Aggregations & GroupBy**: Data analysis and reporting
6. **Indexes & Performance**: Optimization strategies
7. **Advanced Features**: C++20 ranges and coroutines

### API Reference
- **Core**: Database, Table, RecordId
- **Query**: QueryBuilder, expressions, predicates
- **Storage**: StorageEngine, PageManager
- **Index**: BTree, SecondaryIndex
- **Catalog**: System catalog and metadata
- **Reflection**: Compile-time reflection system
- **Serialization**: Binary serialization
- **Ranges**: C++20 ranges integration
- **Coroutines**: Generator and async operations
- **Debug**: Profiler, statistics, execution plans

### Architecture Deep-Dive
- **Overview**: System architecture and design principles
- **Storage Engine**: Page-based storage implementation
- **B-Tree Implementation**: Index algorithms and structure
- **Expression Templates**: Zero-cost abstraction techniques
- **Reflection System**: Compile-time reflection
- **Performance**: Complexity analysis and benchmarks

### Additional Resources
- **C++20 Glossary**: Comprehensive C++20 terminology
- **FAQ**: 50+ frequently asked questions
- **Examples**: 10 complete real-world applications
- **Contributing**: How to contribute to LearnQL

## Editing Documentation

### File Format

All documentation is written in reStructuredText (`.rst`) format.

### Style Guide

- Use clear, beginner-friendly language
- Include code examples for all features
- Add cross-references to related documentation
- Include diagrams where helpful
- Explain "why" not just "how"

### Adding New Pages

1. Create a new `.rst` file in the appropriate directory
2. Add the file to the `toctree` in the parent index or `docs/index.rst`
3. Build and test locally before committing

### Cross-Referencing

```rst
:doc:`/getting-started/installation`  # Link to another document
:ref:`section-label`                   # Link to a section
:class:`learnql::Database`             # Link to API class
```

## Troubleshooting

### "Cannot find class in doxygen xml"

This warning occurs when Breathe can't find the C++ class in Doxygen's XML output. Possible fixes:

1. Ensure Doxygen ran successfully: `ls doxyxml/*.xml` should show many files
2. Check that the class name matches the actual C++ code
3. Verify `EXTRACT_ALL = YES` in Doxyfile

### "Module not found" errors

Install missing Python packages:

```bash
pip3 install -r requirements.txt
```

### Build fails with "command not found"

Ensure the Sphinx binaries are in your PATH:

```bash
export PATH="$HOME/Library/Python/3.11/bin:$PATH"  # macOS
export PATH="$HOME/.local/bin:$PATH"               # Linux
```

### Mermaid diagrams not rendering

Install the mermaid extension:

```bash
pip3 install sphinxcontrib-mermaid
```

## Links

- **ReadTheDocs**: https://learnql.readthedocs.io/
- **GitHub**: https://github.com/tdelphi1981/LearnQL
- **Sphinx Documentation**: https://www.sphinx-doc.org/
- **Doxygen Documentation**: https://www.doxygen.nl/

## Contributing

See `/Users/tolgaberber/Work/CPP/LearnQL/CONTRIBUTING.md` for guidelines on contributing to the documentation.

## License

The documentation is licensed under the same license as LearnQL (Academic Free License 3.0).
