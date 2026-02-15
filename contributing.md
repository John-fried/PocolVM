# Contributing to PocolVM

Thank you for your interest in contributing to PocolVM! This document provides guidelines for contributing to this project.

## Code of Conduct

By participating in this project, you agree to maintain a respectful and inclusive environment.

## Getting Started

### Prerequisites

- C Compiler: GCC 9+ or Clang 10+
- Build System: GNU Make
- Version Control: Git

### Setup

```bash
# Clone the repository
git clone https://github.com/John-fried/PocolVM.git
cd PocolVM

# Build the project
cd posm && make
cd ../pm && make
```

## Development Workflow

### 1. Create a Branch

```bash
git checkout -b feature/your-feature-name
```

### 2. Make Changes

- Follow coding standards
- Add tests for new features
- Update documentation

### 3. Test Your Changes

```bash
cd posm && make
cd ../pm && make
./pm/pm example/3010.pcl
```

## Coding Standards

### File Headers

Every source file should include:

```c
/* filename.c - Short description */

/* Copyright (C) 2026 Bayu Setiawan and Rasya Andrean */

#include <stdio.h>
```

### Naming Conventions

| Type | Convention | Example |
|------|------------|---------|
| Functions | snake_case | pocol_load_program |
| Variables | snake_case | uint64_t pc |
| Constants | UPPER_SNAKE | MAX_BREAKPOINTS |

### Formatting

Use .clang-format:

```bash
make format
```

## Pull Request Process

### Before Submitting

1. Test thoroughly
2. Check formatting: make format
3. Update docs
4. Write tests

### PR Description

Include:
- What the change does
- Why it's needed
- How to test it

## Areas for Contribution

### High Priority
- Bug fixes
- Documentation improvements
- Test coverage

### Medium Priority
- New optimization passes
- Additional system calls

### Low Priority
- New language features
- Performance improvements

## Recognition

Contributors will be recognized in:
- GitHub contributors page
- Release notes

---

Questions? Open an issue or discussion.

Thank you for contributing to PocolVM!