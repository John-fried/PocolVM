# Pocol VM Developer Guide

## Getting Started

### Prerequisites
- **C Compiler**: GCC 4.9+ or Clang 3.5+
- **Build Tools**: GNU Make or compatible make utility
- **Operating System**: Linux, macOS, or Windows
- **Development Environment**: Any text editor or IDE

### Initial Setup

```bash
# Clone the repository
git clone https://github.com/John-fried/PocolVM.git
cd PocolVM

# Build the assembler
cd posm
make
cd ..

# Build the VM with JIT support
cd pm
make
cd ..
```

### Directory Structure for Developers

```
PocolVM/
├── documentation/          # ← You are here
│   ├── PROJECT_OVERVIEW.md
│   ├── TECHNICAL_ARCHITECTURE.md
│   ├── DEVELOPER_GUIDE.md  # ← This file
│   └── API_REFERENCE.md
├── pm/                    # VM Development
│   ├── vm.h              # Core VM interface
│   ├── vm.c              # Core VM implementation
│   ├── jit.h             # JIT compiler interface
│   ├── jit.c             # JIT compiler implementation
│   ├── optimizer.c       # Optimization passes
│   ├── pm.c              # Main application
│   ├── tests/            # Unit and integration tests
│   └── Makefile          # Build configuration
├── posm/                  # Assembler Development
│   ├── compiler.h/c      # Compiler core
│   ├── lexer.h/c         # Lexical analyzer
│   ├── symbol.h/c        # Symbol table
│   ├── emit.h            # Code emitter
│   ├── posm.c            # Main assembler
│   ├── tests/            # Assembler tests
│   └── Makefile          # Build configuration
└── example/               # Example programs
```

## Development Workflow

### 1. Setting Up Your Environment

#### Linux/macOS
```bash
# Install build tools
sudo apt-get install build-essential    # Ubuntu/Debian
brew install gcc make                   # macOS

# Verify installation
gcc --version
make --version
```

#### Windows
```cmd
# Install MinGW-w64 or MSYS2
# Add to PATH environment variable
gcc --version
mingw32-make --version
```

### 2. Building and Testing

```bash
# Clean build
cd pm
make clean
make debug    # Build with debug symbols

# Run tests
./pm tests/jit_test.pob --jit --stats

# Build release version
make clean
make          # Production build with optimizations
```

### 3. Development Cycle

```
1. Modify source code
2. make          # Compile changes
3. Test locally  # Run relevant test cases
4. Debug issues  # Use gdb/lldb if needed
5. Commit changes
6. Push to repository
```

## Code Structure Guidelines

### File Organization
- **Header Files** (.h): Interface declarations only
- **Implementation Files** (.c): Implementation details
- **One Function Per File**: Except for closely related functions
- **Static Functions**: Use `static` for file-local functions

### Naming Conventions

```c
// Functions: verb_noun or noun_verb
pocol_execute_program()
pocol_jit_compile_block()

// Variables: descriptive lowercase with underscores
program_counter
stack_pointer
register_value

// Constants: UPPERCASE with underscores
POCOL_MEMORY_SIZE
INST_HALT
ERR_ILLEGAL_INST

// Types: PascalCase with _t suffix
typedef struct PocolVM PocolVM_t;
typedef enum Inst_Type Inst_Type_t;
```

### Code Documentation

```c
/**
 * Brief description of what the function does
 *
 * @param vm        Pointer to VM context (must not be NULL)
 * @param limit     Maximum instructions to execute (-1 for unlimited)
 * @return          Error code (ERR_OK on success)
 *
 * Detailed description explaining:
 * - Pre-conditions and assumptions
 * - Side effects
 * - Error conditions
 * - Usage examples
 */
Err pocol_execute_program(PocolVM *vm, int limit);
```

## Core Development Areas

### 1. Virtual Machine Core

#### Key Files to Understand
- `vm.h`: Core data structures and interfaces
- `vm.c`: Main execution engine implementation
- `common.h`: Shared macros and definitions

#### Common Modification Tasks

**Adding a New Instruction:**
```c
// 1. Add to instruction enum in vm.h
typedef enum {
    INST_HALT = 0,
    INST_PUSH,
    INST_POP,
    INST_ADD,
    INST_SUB,        // ← New instruction
    INST_JMP,
    INST_PRINT,
    COUNT_INST
} Inst_Type;

// 2. Add to instruction table in compiler.c
const Inst_Def inst_table[COUNT_INST] = {
    [INST_SUB] = { .type = INST_SUB, .name = "sub", .operand = 2 },
};

// 3. Implement execution logic in vm.c
case INST_SUB: {
    uint64_t *dest = &vm->registers[REG_OP(NEXT)];
    uint64_t src = pocol_fetch_operand(vm, op2);
    *dest -= src;  // Subtraction instead of addition
} break;
```

### 2. JIT Compiler Development

#### Key Files
- `jit.h`: JIT interfaces and data structures
- `jit.c`: Core JIT compilation logic
- `optimizer.c`: Optimization passes

#### JIT Development Workflow

```c
// 1. Analyze bytecode pattern
// 2. Generate equivalent x86-64 code
// 3. Handle register mapping
// 4. Manage memory allocation
// 5. Test thoroughly

// Example: Adding x86-64 instruction helper
static inline void emit_sub_reg_reg(uint8_t **code_ptr, uint8_t dst_reg, uint8_t src_reg) {
    emit_byte(code_ptr, 0x48);  // REX.W prefix
    emit_byte(code_ptr, 0x29);  // SUB reg, reg
    emit_byte(code_ptr, 0xC0 + (src_reg << 3) + dst_reg);
}
```

### 3. Assembler Development

#### Key Files
- `lexer.h/c`: Token generation
- `compiler.h/c`: Parsing and code generation
- `symbol.h/c`: Symbol resolution

#### Common Tasks

**Adding Parser Support:**
```c
// Add new token type
typedef enum {
    TOK_EOF = 0,
    TOK_IDENTIFIER,
    TOK_INTEGER,
    TOK_REGISTER,
    TOK_NEW_FEATURE,    // ← New token
} TokenType;

// Add parsing logic
if (ctx->lookahead.type == TOK_NEW_FEATURE) {
    // Handle new syntax
    parser_advance(ctx);
    // Generate appropriate bytecode
}
```

## Debugger

### Overview
The debugger provides step-by-step execution control with breakpoints, watchpoints, and state inspection.

### Key Files
- `vm_debugger.h`: Debugger interface and data structures
- `vm_debugger.c`: Debugger implementation

### Features
- Breakpoints (64 max)
- Watchpoints for memory access
- Step in/over/out execution
- Register and memory inspection
- Disassembly
- Call stack visualization

### Usage
```bash
# Enter debugger
./pm/pm program.pob --debug

# Debugger commands
(pocol-debug) s           # step
(pocol-debug) n           # next (step over)
(pocol-debug) c           # continue
(pocol-debug) p           # print registers
(pocol-debug) x/16 0x100  # examine memory
(pocol-debug) break 20   # set breakpoint
(pocol-debug) q           # quit
```

## System Calls

### Overview
System calls provide VM-to-host OS integration for file I/O, process management, and more.

### Key Files
- `vm_syscalls.h`: System call definitions
- `vm_syscalls.c`: System call implementations

### Available System Calls

| Number | Name | Description |
|--------|------|-------------|
| 1 | SYS_PRINT | Print to console |
| 2 | SYS_READ | Read from console |
| 3 | SYS_OPEN | Open file |
| 4 | SYS_CLOSE | Close file |
| 5 | SYS_WRITE | Write to file |
| 6 | SYS_READ_FILE | Read from file |
| 7 | SYS_SEEK | Seek in file |
| 8 | SYS_TELL | Get file position |
| 11 | SYS_TIME | Get current time |
| 12 | SYS_SLEEP | Sleep (milliseconds) |
| 13 | SYS_EXIT | Exit program |
| 17 | SYS_CHDIR | Change directory |
| 18 | SYS_GETCWD | Get current directory |
| 22 | SYS_MKDIR | Create directory |
| 25 | SYS_SYSTEM | Execute shell command |

### Usage in Assembly
```assembly
; Call SYS_TIME (11)
push 11
pop r0
sys
print r0
```

## Virtual File System

### Overview
VFS provides virtual file descriptors with host OS integration.

### Features
- 256 file descriptors
- Special files: /dev/stdin, /dev/stdout, /dev/stderr
- Host file system integration
- Directory operations

### Key Structures
```c
typedef struct VFile {
    char name[64];
    char path[256];
    uint8_t type;
    uint64_t size;
    uint64_t position;
    bool is_open;
    // ...
} VFile;
```

## PocolC Compiler

### Overview
PocolC is a high-level language that compiles to PocolVM bytecode.

### Key Files
- `poclc.h`: Lexer, parser, AST definitions
- `poclc.c`: Implementation
- `poclc_main.c`: Main entry point

### Language Features
- Variables and assignment
- Arithmetic operations (+, -, *, /, %)
- Comparison operators (==, !=, <, >, <=, >=)
- If-else statements
- While/for loops
- Functions with parameters and return values
- Recursion

### Usage
```bash
# Compile PocolC to bytecode
./poclc/poclc program.pc -o program.pob

# Run the program
./pm/pm program.pob
```

### Example Program
```c
func factorial(n) {
    var result = 1;
    var i = 1;
    while (i <= n) {
        result = result * i;
        i = i + 1;
    }
    return result;
}

func main() {
    print factorial(5);
}
```

## Testing Strategy

### Unit Testing Approach

```bash
# Test individual components
cd pm/tests

# Simple functionality test
../pm simple_test.pob

# JIT compilation test
../pm jit_test.pob --jit

# Performance benchmark
../pm benchmark.pob --jit --stats
```

### Writing Test Cases

Create `.pcl` test files in `tests/` directory:

```assembly
; tests/arithmetic_test.pcl
_start:
    push 15
    push 25
    pop r0      ; r0 = 25
    pop r1      ; r1 = 15
    add r0, r1  ; r0 = 40
    print r0    ; Should output: 40
    halt
```

### Debugging Techniques

#### Using GDB/LLDB
```bash
# Compile with debug symbols
cd pm
make debug

# Debug execution
gdb ./pm
(gdb) run tests/debug_test.pob
(gdb) bt          # Backtrace
(gdb) info regs  # Register contents
(gdb) disas      # Disassemble current function
```

#### Debug Output
```c
// Add debug prints
#ifdef DEBUG
    printf("PC: %u, Instruction: %u\n", vm->pc, op);
#endif
```

## Performance Optimization

### Profiling Tools

#### Linux/macOS
```bash
# Compile with profiling
make CFLAGS="-pg"

# Run with profiling
./pm benchmark.pob --jit

# Analyze results
gprof pm gmon.out > profile.txt
```

#### General Optimization Tips
1. **Profile First**: Identify actual bottlenecks
2. **Algorithmic Improvements**: Focus on O() complexity
3. **Memory Access Patterns**: Cache-friendly layouts
4. **Branch Prediction**: Minimize unpredictable branches
5. **Compiler Optimizations**: Use appropriate flags

### Common Performance Issues

```c
// ❌ Slow: Repeated bounds checking
for (int i = 0; i < 1000; i++) {
    if (pc + i >= POCOL_MEMORY_SIZE) return ERR_ILLEGAL_INST_ACCESS;
    // Process memory[pc + i]
}

// ✅ Fast: Single bounds check
if (pc + 1000 >= POCOL_MEMORY_SIZE) return ERR_ILLEGAL_INST_ACCESS;
for (int i = 0; i < 1000; i++) {
    // Process memory[pc + i] safely
}
```

## Code Quality Standards

### Static Analysis
```bash
# Enable compiler warnings
make CFLAGS="-Wall -Wextra -Wpedantic -Werror"

# Use static analyzers
cppcheck *.c
clang-analyzer *.c
```

## Topics

### Adding New Optimization Passes

```c
// 1. Define optimization function
Err pocol_opt_loop_unrolling(PocolVM *vm) {
    // Implementation
    return ERR_OK;
}

// 2. Add to optimization pipeline
Err pocol_optimize_bytecode(PocolVM *vm, OptLevel level) {
    // ... existing optimizations ...
    
    if (level >= OPT_LEVEL_ADVANCED) {
        err = pocol_opt_loop_unrolling(vm);
        if (err != ERR_OK) return err;
    }
    
    return ERR_OK;
}
```

### Extending the Instruction Set

Considerations when adding instructions:
1. **Semantic Clarity**: Clear, unambiguous behavior
2. **Performance Impact**: Efficient implementation
3. **JIT Support**: Native code generation
4. **Backward Compatibility**: Existing code still works
5. **Documentation**: Complete specification

### Memory Management Improvements

Future enhancements:
- Generational garbage collection
- Memory pooling for frequent allocations
- Copy-on-write for immutable data
- Arena allocators for related objects

## 🤝 Contributing Guidelines

### Pull Request Process
1. Fork the repository
2. Create feature branch (`git checkout -b feature/amazing-feature`)
3. Commit changes (`git commit -m 'Add amazing feature'`)
4. Push to branch (`git push origin feature/amazing-feature`)
5. Open Pull Request

### Code Review Standards
- **Function Length**: Keep functions under 50 lines when possible
- **Parameter Count**: Limit to 4 parameters per function
- **Nesting Depth**: Avoid deeply nested conditionals
- **Magic Numbers**: Use named constants
- **Comments**: Explain why, not what

### Issue Reporting Template
```markdown
## Bug Report

**Description**: Clear description of the issue

**Steps to Reproduce**:
1. Build with 'make'
2. Run './pm test.pob'
3. Observe unexpected behavior

**Expected Behavior**: What should happen

**Actual Behavior**: What actually happens

**Environment**:
- OS: [e.g., Ubuntu 20.04]
- Compiler: [e.g., GCC 9.3.0]
- Version: [e.g., v1.0.0]

**Additional Context**: Screenshots, logs, etc.
```

---

*Copyright (C) 2026 Bayu Setiawan and Rasya Andrean*
*Developer Guide Version: 1.0*  
*Last Updated: February 2026*