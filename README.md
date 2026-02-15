# Pocol VM

[![Language](https://img.shields.io/badge/language-C-blue.svg?style=flat-square)](https://en.wikipedia.org/wiki/C_(programming_language))
[![License](https://img.shields.io/badge/license-MIT-green.svg?style=flat-square)](LICENSE)
[![Build](https://img.shields.io/badge/build-Makefile-orange.svg?style=flat-square)](pm/Makefile)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey.svg?style=flat-square)](#)
[![Status](https://img.shields.io/badge/status-Active-brightgreen.svg?style=flat-square)](#)

Pocol is a minimalist, register-based [Virtual Machine](https://en.wikipedia.org/wiki/Virtual_machine) with a complete toolchain including assembler, high-level compiler, JIT compilation, and debugger.

## Features

* **Complete Toolchain**: From assembly to high-level language (PocolC)
* **Minimalist Design**: Clean, simple architecture focused on essentials
* **Register-based**: 8 general-purpose 64-bit registers for efficient computation
* **Stack Operations**: 1024-slot stack for flexible data manipulation
* **Cross-platform**: Builds and runs on Linux, macOS, and Windows
* **JIT Compilation**: Compile bytecode directly to x86-64 machine code
* **Bytecode Optimizer**: Constant folding, dead code elimination, peephole optimizations
* **Debugger**: Step-by-step execution with breakpoints, watchpoints, and visualizer
* **System Calls**: Virtual file system, file I/O, timers, process management
* **Zero Dependencies**: Pure C implementation with no external libraries

## Architecture Overview

### Core Components
* **Registers**: 8 general-purpose 64-bit registers (`r0`-`r7`)
* **Stack**: 1024 slots for 64-bit integer operations
* **Memory**: 640KB linear address space
* **Word Size**: 64-bit [Little-Endian](https://en.wikipedia.org/wiki/Endianness) format
* **Instruction Set**: Minimal but extensible instruction set

### Enhanced Features
* **JIT Compiler**: Runtime compilation to native x86-64 code
* **Optimization Passes**: Multi-level bytecode optimization
* **Debugger**: Breakpoints, step in/over/out, register/memory watch
* **VFS**: Virtual file system with host OS integration
* **System Calls**: 25+ system calls for file I/O, time, process management

## Project Structure

```
PocolVM/
├── pm/                      # VM Core
│   ├── vm.h, vm.c         # VM core definitions & implementation
│   ├── jit.h, jit.c       # JIT compilation
│   ├── optimizer.c         # Bytecode optimizer
│   ├── vm_debugger.h/c    # Debugger
│   ├── vm_syscalls.h/c    # System calls & VFS
│   ├── benchmark.c         # Benchmark suite
│   ├── pm.c               # Main executable
│   ├── Makefile           # Build configuration
│   └── tests/             # Test programs (.pcl)
├── posm/                   # Assembler
│   ├── compiler.h, c       # Compiler implementation
│   ├── lexer.h, c         # Lexer
│   ├── symbol.h, c        # Symbol table
│   ├── emit.h             # Code emission
│   ├── posm.c             # Main assembler
│   ├── Makefile           # Build configuration
│   └── tests/             # Test cases
├── poclc/                  # High-level compiler (PocolC)
│   ├── poclc.h            # Lexer/parser/AST definitions
│   ├── poclc.c            # Implementation
│   ├── poclc_main.c       # Main entry point
│   ├── Makefile           # Build configuration
│   └── tests/             # Test programs (.pc)
├── testing/                # Test framework
│   ├── test_framework.h   # Test macros
│   └── test_framework.c   # Implementation
├── documentation/          # Documentation
│   ├── INDEX.md           # Navigation hub
│   ├── PROJECT_OVERVIEW.md
│   ├── TECHNICAL_ARCHITECTURE.md
│   ├── DEVELOPER_GUIDE.md
│   ├── API_REFERENCE.md
│   └── USER_MANUAL.md
├── .github/workflows/      # CI/CD
│   └── ci.yml             # GitHub Actions
├── .clang-format          # Code formatter config
├── .cppcheck              # Static analysis config
├── example/               # Example programs
├── common.h               # Shared definitions
└── README.md              # This file
```

## Building

### Prerequisites
* **GCC** or compatible C compiler (C99 standard)
* **Make** build system

### Build Process

```bash
# Build the assembler
cd posm && make

# Build the VM (with JIT support)
cd ../pm && make

# Build the PocolC compiler (optional)
cd ../poclc && make
```

## ▶Usage

### Toolchain

```bash
# Assembly source (.pcl) → Bytecode (.pob)
./posm/posm program.pcl -o program.pob

# High-level (.pc) → Bytecode (.pob)
./poclc/poclc program.pc -o program.pob

# Run bytecode
./pm/pm program.pob
```

### Execution Modes

```bash
# Interpreter mode (default)
./pm/pm program.pob

# JIT compilation mode
./pm/pm program.pob --jit

# With statistics
./pm/pm program.pob --jit --stats

# Debugger mode
./pm/pm program.pob --debug

# With initial breakpoint
./pm/pm program.pob --debug --break=10
```

### Command Line Options

| Option | Description |
|--------|-------------|
| `<program.pob>` | Input bytecode file (required) |
| `[limit]` | Maximum instruction count |
| `--jit` | Enable JIT compilation |
| `--stats` | Display statistics |
| `--debug` | Enable debugger |
| `--break=ADDR` | Set initial breakpoint |

### Debugger Commands

| Command | Description |
|---------|-------------|
| `s`, `step` | Step one instruction |
| `n`, `next` | Step over |
| `c`, `continue` | Continue execution |
| `p`, `print` | Show registers |
| `bt` | Show call stack |
| `x/N ADDR` | Examine memory |
| `break ADDR` | Set breakpoint |
| `q`, `quit` | Quit |

## Performance

### When JIT Shines
✅ Compute-intensive loops
✅ Programs with repetitive patterns
✅ Long-running applications
✅ Mathematical computations

### When Interpreter is Better
⚠️ Very short programs
⚠️ Frequent control flow changes
⚠️ Memory-constrained environments
⚠️ Debugging and development

## Testing

```bash
# Test assembler
cd posm && make
./posm example/3010.pcl -o /tmp/test.pob

# Test VM
cd ../pm && make
./pm /tmp/test.pob

# Run with JIT
./pm /tmp/test.pob --jit

# Run debugger
./pm /tmp/test.pob --debug

# Run PocolC tests
cd ../poclc && make
./poclc tests/hello.pc -o tests/hello.pob
```

## 🤝 Contributing

Areas for improvement:
* Additional optimization passes
* More sophisticated JIT compilation
* Extended instruction set
* Better error handling
* Cross-platform improvements


```

## Documentation

- [Complete Documentation](documentation/INDEX.md)
- [User Manual](documentation/USER_MANUAL.md)
- [Developer Guide](documentation/DEVELOPER_GUIDE.md)
- [API Reference](documentation/API_REFERENCE.md)
- [Technical Architecture](documentation/TECHNICAL_ARCHITECTURE.md)

---

## Badges & Metrics

[![GitHub](https://img.shields.io/github/languages/code-size/John-fried/PocolVM?style=flat-square)](https://github.com/John-fried/PocolVM)
[![Maintenance](https://img.shields.io/maintenance/yes/2026?style=flat-square)](https://github.com/John-fried/PocolVM)
[![Commits](https://img.shields.io/github/commit-activity/m/John-fried/PocolVM?style=flat-square)](https://github.com/John-fried/PocolVM/commits)
[![Issues](https://img.shields.io/github/issues/John-fried/PocolVM?style=flat-square)](https://github.com/John-fried/PocolVM/issues)
