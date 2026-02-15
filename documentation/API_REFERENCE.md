# Pocol VM API Reference

## Overview

This document provides comprehensive reference documentation for the Pocol VM API. All functions, data structures, and constants are documented with their signatures, parameters, return values, and usage examples.

## Core Data Structures

### PocolVM
The main virtual machine context structure.

```c
typedef struct {
    /* Basic VM State */
    uint8_t    memory[POCOL_MEMORY_SIZE];   // 640KB linear memory space
    Inst_Addr  pc;                          // Program counter (0-65535)
    uint64_t   stack[POCOL_STACK_SIZE];     // 1024-slot stack
    Stack_Addr sp;                          // Stack pointer (0-1023)
    uint64_t   registers[8];                // 8 general-purpose registers (r0-r7)
    unsigned int halt : 1;                  // Halt flag (1 = stopped)
    
    /* JIT Support */
    void *jit_context;                      // Opaque pointer to JIT context
} PocolVM;
```

**Constants:**
- `POCOL_MEMORY_SIZE`: 640000 bytes (640KB)
- `POCOL_STACK_SIZE`: 1024 slots
- `POCOL_MAGIC`: 0x6f636f70 ('poco' in reverse)

### Error Codes
Enumeration of possible error conditions.

```c
typedef enum {
    ERR_OK = 0,              // Success
    ERR_ILLEGAL_INST,        // Unrecognized opcode
    ERR_ILLEGAL_INST_ACCESS, // Memory access violation
    ERR_STACK_OVERFLOW,      // Stack exceeded capacity
    ERR_STACK_UNDERFLOW      // Stack empty when pop attempted
} Err;
```

### Instruction Types
Supported instruction set enumeration.

```c
typedef enum {
    INST_HALT = 0,           // Stop execution
    INST_PUSH,               // Push value to stack
    INST_POP,                // Pop value from stack to register
    INST_ADD,                // Add two values
    INST_JMP,                // Unconditional jump
    INST_PRINT,              // Print value (debug)
    COUNT_INST               // Total instruction count
} Inst_Type;
```

### Operand Types
Specification of operand formats.

```c
typedef enum {
    OPR_NONE = 0,            // No operand
    OPR_REG = 0x01,          // Register operand (r0-r7)
    OPR_IMM = 0x02           // Immediate value operand
} OperandType;
```

## Core VM Functions

### pocol_load_program_into_vm()
Load a compiled program into the virtual machine.

```c
int pocol_load_program_into_vm(const char *path, PocolVM **vm);
```

**Parameters:**
- `path`: Path to `.pob` bytecode file
- `vm`: Pointer to VM pointer (will be allocated)

**Returns:**
- `0`: Success
- `-1`: Error (check errno for details)

**Example:**
```c
PocolVM *vm = NULL;
if (pocol_load_program_into_vm("program.pob", &vm) == 0) {
    // Successfully loaded
    pocol_execute_program(vm, -1);  // Execute indefinitely
    pocol_free_vm(vm);
} else {
    perror("Failed to load program");
}
```

### pocol_free_vm()
Release all resources associated with a VM instance.

```c
void pocol_free_vm(PocolVM *vm);
```

**Parameters:**
- `vm`: VM instance to free (can be NULL)

**Notes:**
- Also frees associated JIT context if present
- Safe to call multiple times
- Sets pointer to NULL after freeing

### pocol_execute_program()
Execute a loaded program in the virtual machine.

```c
Err pocol_execute_program(PocolVM *vm, int limit);
```

**Parameters:**
- `vm`: Initialized VM instance
- `limit`: Maximum instructions to execute (-1 for unlimited)

**Returns:**
- `ERR_OK`: Successful execution
- Other: Error code indicating failure reason

**Example:**
```c
// Execute 1000 instructions maximum
Err result = pocol_execute_program(vm, 1000);
if (result != ERR_OK) {
    fprintf(stderr, "Execution failed: %s\n", err_as_cstr(result));
}
```

### pocol_execute_inst()
Execute a single instruction.

```c
Err pocol_execute_inst(PocolVM *vm);
```

**Parameters:**
- `vm`: VM instance with loaded program

**Returns:**
- `ERR_OK`: Instruction executed successfully
- Error code: Execution failed

**Usage:**
```c
// Manual instruction-by-instruction execution
while (!vm->halt) {
    Err err = pocol_execute_inst(vm);
    if (err != ERR_OK) {
        handle_error(err);
        break;
    }
    // Inspect VM state between instructions
}
```

### pocol_execute_program_jit()
Execute program with optional JIT compilation.

```c
Err pocol_execute_program_jit(PocolVM *vm, int limit, int jit_enabled);
```

**Parameters:**
- `vm`: Initialized VM instance
- `limit`: Instruction limit (-1 for unlimited)
- `jit_enabled`: Non-zero to enable JIT compilation

**Returns:**
- `ERR_OK`: Successful execution
- Error code: Execution failed

**Example:**
```c
// With JIT compilation
Err result = pocol_execute_program_jit(vm, -1, 1);  // Enable JIT

// Interpreter mode
Err result = pocol_execute_program_jit(vm, -1, 0);  // Disable JIT
```

## JIT Compilation API

### JitMode
Execution mode enumeration for JIT compiler.

```c
typedef enum {
    JIT_MODE_DISABLED = 0,   // Use interpreter only
    JIT_MODE_ENABLED,        // Compile and execute native code
    JIT_MODE_TRACE           // Trace execution and compile hot paths
} JitMode;
```

### OptLevel
Optimization level specification.

```c
typedef enum {
    OPT_LEVEL_NONE = 0,      // No optimization
    OPT_LEVEL_BASIC,         // Constant folding, dead code elimination
    OPT_LEVEL_ADVANCED       // All basic + peephole optimizations
} OptLevel;
```

### pocol_jit_init()
Initialize JIT compilation context.

```c
void pocol_jit_init(JitContext *jit_ctx, JitMode mode, OptLevel opt_level);
```

**Parameters:**
- `jit_ctx`: JIT context to initialize
- `mode`: Execution mode
- `opt_level`: Optimization level

**Example:**
```c
JitContext jit_ctx;
pocol_jit_init(&jit_ctx, JIT_MODE_ENABLED, OPT_LEVEL_BASIC);
```

### pocol_jit_free()
Release JIT compilation resources.

```c
void pocol_jit_free(JitContext *jit_ctx);
```

**Parameters:**
- `jit_ctx`: JIT context to release

### pocol_jit_execute_program()
Execute program using JIT compilation.

```c
Err pocol_jit_execute_program(JitContext *jit_ctx, PocolVM *vm, int limit);
```

**Parameters:**
- `jit_ctx`: Initialized JIT context
- `vm`: VM with loaded program
- `limit`: Instruction limit

**Returns:**
- `ERR_OK`: Successful execution
- Error code: Execution failed

### pocol_jit_print_stats()
Display JIT compilation statistics.

```c
void pocol_jit_print_stats(JitContext *jit_ctx);
```

**Output Example:**
```
=== JIT Statistics ===
Mode: Enabled
Optimization Level: Basic
Compiled blocks: 3
Executed blocks: 1500000
Cache entries: 3/256
Code buffer used: 1024/1048576 bytes
```

## Optimization API

### pocol_optimize_bytecode()
Apply bytecode optimizations.

```c
Err pocol_optimize_bytecode(PocolVM *vm, OptLevel level);
```

**Parameters:**
- `vm`: VM containing bytecode to optimize
- `level`: Optimization level to apply

**Returns:**
- `ERR_OK`: Optimization successful
- Error code: Optimization failed

### pocol_opt_fold_constants()
Apply constant folding optimization.

```c
Err pocol_opt_fold_constants(PocolVM *vm);
```

**Optimizations Applied:**
- Evaluate constant expressions at compile time
- Combine adjacent constant operations
- Eliminate redundant calculations

### pocol_opt_eliminate_dead_code()
Remove unreachable or ineffective code.

```c
Err pocol_opt_eliminate_dead_code(PocolVM *vm);
```

**Optimizations Applied:**
- Remove instructions with no observable effects
- Eliminate unreachable code paths
- Remove redundant assignments

### pocol_opt_peephole()
Apply local pattern-based optimizations.

```c
Err pocol_opt_peephole(PocolVM *vm);
```

**Optimizations Applied:**
- Simplify instruction sequences
- Eliminate redundant operations
- Optimize common patterns

## Utility Functions

### pocol_error()
Formatted error output function.

```c
void pocol_error(const char *fmt, ...);
```

**Parameters:**
- `fmt`: Printf-style format string
- `...`: Variable arguments

**Example:**
```c
pocol_error("Invalid instruction at PC %u\n", vm->pc);
pocol_error("Stack overflow detected (size: %u)\n", vm->sp);
```

### err_as_cstr()
Convert error code to human-readable string.

```c
char *err_as_cstr(Err err);
```

**Parameters:**
- `err`: Error code to convert

**Returns:**
- String representation of error code

**Example:**
```c
Err result = pocol_execute_program(vm, 100);
if (result != ERR_OK) {
    printf("Error: %s\n", err_as_cstr(result));
}
```

## Helper Macros

### Memory and Stack Management
```c
#define POCOL_MEMORY_SIZE    (640 * 1000)    // 640KB memory
#define POCOL_STACK_SIZE     1024            // 1024 stack slots
#define POCOL_MAGIC          0x6f636f70      // File magic number

#define DESC_PACK(op1, op2)  (((op2) << 4) | (op1))  // Pack descriptor
#define DESC_GET_OP1(desc)   ((desc) & 0x0F)         // Extract op1
#define DESC_GET_OP2(desc)   ((desc) >> 4)           // Extract op2
```

### Register Operations
```c
#define REG_OP(operand)      (operand & 0x07)        // Get register index
#define NEXT                 (vm->memory[vm->pc++])  // Fetch next byte
```

## Performance Considerations

### Memory Layout
```c
// VM memory organization
0x00000000: Magic header (4 bytes)
0x00000004: First instruction
...         Program bytecode
0x0009C400: End of memory (640KB)
```

### Stack Operations
```c
// Push operation
if (vm->sp >= POCOL_STACK_SIZE) return ERR_STACK_OVERFLOW;
vm->stack[vm->sp++] = value;

// Pop operation
if (vm->sp == 0) return ERR_STACK_UNDERFLOW;
value = vm->stack[--vm->sp];
```

### Register Access
```c
// Direct register access
uint64_t value = vm->registers[reg_index & 0x07];  // Bounds-safe

// Register assignment
vm->registers[reg_index & 0x07] = new_value;
```

## Thread Safety

**Important Notes:**
- VM instances are **not thread-safe**
- Each thread must use separate VM instances
- JIT contexts are tied to specific VM instances
- Concurrent access requires external synchronization

## Version Compatibility

### API Stability
- **Core VM API**: Stable (v1.0)
- **JIT API**: Evolving (subject to change)
- **Optimizer API**: Experimental (may change significantly)

### Backward Compatibility
- Existing `.pob` files remain compatible
- Core instruction set is frozen
- New instructions added as extensions
- Deprecated functions marked with warnings

## Error Handling Best Practices

```c
// Always check return values
Err result = pocol_load_program_into_vm("program.pob", &vm);
if (result != 0) {
    // Handle error appropriately
    fprintf(stderr, "Failed to load program: %s\n", strerror(errno));
    return -1;
}

// Check execution results
result = pocol_execute_program_jit(vm, -1, 1);
if (result != ERR_OK) {
    fprintf(stderr, "Execution failed at PC %u: %s\n", 
            vm->pc, err_as_cstr(result));
    pocol_free_vm(vm);
    return -1;
}
```

---

*Copyright (C) 2026 Bayu Setiawan and Rasya Andrean*
*API Reference Version: 1.0*  
*Last Updated: February 2026*