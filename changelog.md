# PocolVM Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [1.0.0] - 2026-02-14 [Development]

### Added

#### Core VM
- **Register-based Architecture**: 8 general-purpose 64-bit registers (r0-r7)
- **Stack Operations**: 1024-slot stack for 64-bit integer operations
- **Linear Memory**: 640KB address space
- **Instruction Set**: HALT, PUSH, POP, ADD, JMP, PRINT, SYS

#### JIT Compilation
- **x86-64 Machine Code Generation**: Runtime compilation of bytecode to native machine code
- **Trace-based JIT**: Optimized compilation of frequently executed traces
- **JIT Cache**: Efficient caching of compiled code
- **Platform Support**: Linux (mmap), macOS (mmap), Windows (VirtualAlloc)

#### Bytecode Optimizer
- **Constant Folding**: Compile-time evaluation of constant expressions
- **Dead Code Elimination**: Removal of unreachable code
- **Peephole Optimizations**: Local instruction sequence improvements
- **Optimization Levels**: NONE, BASIC, ADVANCED

#### Debugger
- **Breakpoints**: Support for up to 64 breakpoints
- **Watchpoints**: Memory access monitoring (read/write/access)
- **Step Execution**: Step in, step over, step out
- **State Inspection**: Register and memory examination
- **Disassembly**: Instruction-level code visualization
- **Call Stack**: Function call tracking
- **Visualizer**: Real-time state display

#### System Calls (25+ calls)
- **Console I/O**: SYS_PRINT, SYS_READ
- **File Operations**: SYS_OPEN, SYS_CLOSE, SYS_WRITE, SYS_READ_FILE, SYS_SEEK, SYS_TELL
- **Process Management**: SYS_EXIT, SYS_SYSTEM
- **Time**: SYS_TIME, SYS_SLEEP
- **Directory Operations**: SYS_CHDIR, SYS_GETCWD, SYS_MKDIR

#### Virtual File System
- **256 File Descriptors**: Virtual file descriptor table
- **Special Files**: /dev/stdin, /dev/stdout, /dev/stderr
- **Host Integration**: Direct access to host file system
- **Directory Support**: Open, read, close directory entries

#### PocolC High-Level Compiler
- **Lexer**: Tokenization of PocolC source code
- **Parser**: AST generation from tokens
- **Code Generator**: Translation to PocolVM bytecode

##### Language Features
- Variables and assignment (`var x = 5;`)
- Arithmetic operators (`+`, `-`, `*`, `/`, `%`)
- Comparison operators (`==`, `!=`, `<`, `>`, `<=`, `>=`)
- Control flow (`if`, `else`, `while`, `for`)
- Functions with parameters and return values
- Recursion support

#### Toolchain
- **posm**: Assembler (.pcl to .pob)
- **poclc**: High-level compiler (.pc to .pob)
- **pm**: Virtual machine runtime

#### Testing & Quality
- **Unit Test Framework**: Simple assertion-based testing
- **Benchmark Suite**: Performance measurement tools
- **GitHub Actions CI/CD**: Automated build and test pipeline
- **Code Formatting**: .clang-format configuration
- **Static Analysis**: .cppcheck configuration

### Documentation
- **User Manual**: End-user guide
- **Developer Guide**: Contributor documentation
- **API Reference**: Technical function reference
- **Technical Architecture**: System design documentation
- **Project Overview**: Philosophy and goals

### Project Structure
- pm/ - VM Core with JIT, Optimizer, Debugger, System Calls
- posm/ - Assembler
- poclc/ - High-level compiler
- testing/ - Test framework
- documentation/ - Complete documentation
- .github/workflows/ - CI/CD

### Copyright
- All source files include dual copyright: Bayu Setiawan and Rasya Andrean

### Known Limitations
- JIT compilation requires executable memory allocation
- Some security policies may restrict JIT functionality
- Limited to 64-bit integer operations

---

## [0.0.0] - 2026-02-13 [Initial]

### Added
- Initial project setup
- Basic VM implementation
- Simple assembler