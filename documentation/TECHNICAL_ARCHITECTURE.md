# Pocol VM Technical Architecture

## System Architecture

### High-Level Overview

```
┌─────────────────┐    ┌──────────────────┐    ┌──────────────────┐
│   Source Code   │───▶│    Assembler     │───▶│   Bytecode (.pob)│
│    (.pcl files) │    │     (posm)       │    │                  │
└─────────────────┘    └──────────────────┘    └─────────┬────────┘
                                                         │
                                                         ▼
┌─────────────────┐    ┌──────────────────┐    ┌──────────────────┐
│   Statistics    │◀───│  Virtual Machine │◀───│   Execution      │
│     Output      │    │      (pm)        │    │    Engine        │
└─────────────────┘    └─────────┬────────┘    └──────────────────┘
                                 │
                    ┌────────────┴────────────┐
                    │                         │
            ┌───────────────┐       ┌──────────────────┐
            │   Interpreter │       │   JIT Compiler   │
            │     Mode      │       │      Mode        │
            └───────────────┘       └─────────┬────────┘
                                              │
                                    ┌─────────▼─────────┐
                                    │  Native x86-64    │
                                    │     Code          │
                                    └───────────────────┘
```

## Core Components

### 1. Virtual Machine (pm/)

#### Main Components
- **VM Core** (`vm.h/vm.c`): Central execution engine
- **JIT Compiler** (`jit.h/jit.c`): Runtime native code generation
- **Optimizer** (`optimizer.c`): Bytecode optimization passes
- **Main Driver** (`pm.c`): Command-line interface and execution control

#### Data Structures

```c
typedef struct {
    /* Basic VM State */
    uint8_t    memory[POCOL_MEMORY_SIZE];   // 640KB linear memory
    Inst_Addr  pc;                          // Program counter
    uint64_t   stack[POCOL_STACK_SIZE];     // 1024-slot stack
    Stack_Addr sp;                          // Stack pointer
    uint64_t   registers[8];                // 8 general-purpose registers
    unsigned int halt : 1;                  // Halt flag
    
    /* JIT Support */
    void *jit_context;                      // Opaque JIT context pointer
} PocolVM;
```

### 2. Assembler (posm/)

#### Component Architecture
- **Lexer** (`lexer.h/lexer.c`): Token generation from source
- **Parser** (`compiler.h/compiler.c`): Syntax analysis and AST generation
- **Symbol Table** (`symbol.h/symbol.c`): Label and identifier management
- **Code Generator** (`emit.h`): Bytecode emission utilities
- **Main Assembler** (`posm.c`): Orchestration and file I/O

## Instruction Set Architecture

### Supported Instructions

| Instruction | Operands | Description |
|-------------|----------|-------------|
| `HALT` | None | Stop VM execution |
| `PUSH` | src | Push value to stack |
| `POP` | dst | Pop value from stack to register |
| `ADD` | dst, src | Add src to dst register |
| `JMP` | target | Unconditional jump |
| `PRINT` | src | Print value (debug) |

### Operand Types
- **Register** (`OPR_REG`): One of r0-r7 registers
- **Immediate** (`OPR_IMM`): 64-bit integer literal
- **None** (`OPR_NONE`): No operand

### Instruction Encoding
```
[Byte 0]     [Byte 1]     [Bytes 2+]    
Opcode       Descriptor   Operands      
(1 byte)     (1 byte)     (Variable)    

Descriptor Format:
Bits 3-0: First operand type
Bits 7-4: Second operand type
```

## JIT Compilation Architecture

### Compilation Pipeline

```
Bytecode ──▶ Analysis ──▶ Optimization ──▶ Code Generation ──▶ Native Code
```

### Key Stages

#### 1. Analysis Phase
- Instruction decoding and validation
- Control flow graph construction
- Register usage analysis
- Memory access pattern recognition

#### 2. Optimization Phase
- **Level 1 (Basic)**: Constant folding, dead code elimination
- **Level 2 (Advanced)**: Peephole optimizations, pattern matching
- **Level 3 (Future)**: Register allocation, loop optimization

#### 3. Code Generation
- x86-64 instruction selection
- Register mapping (Pocol → x86-64)
- Memory management for generated code
- Platform-specific calling conventions

### Memory Management

#### Executable Memory Allocation
```c
// Linux/macOS
mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, 
     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

// Windows  
VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, 
             PAGE_EXECUTE_READWRITE);
```

#### Cache Management
- **Cache Size**: 256 entries maximum
- **Entry Structure**: PC range + compiled code + statistics
- **Replacement Policy**: Simple FIFO (future: LRU)
- **Memory Layout**: Contiguous buffer with dynamic allocation

## Optimization Strategies

### 1. Constant Folding
```assembly
; Before optimization
add r0, 10
add r0, 20
; After: add r0, 30
```

### 2. Dead Code Elimination
```assembly
; Before optimization
push 0
pop r1
add r1, 0    ; This instruction has no effect
; After: Removed entirely
```

### 3. Peephole Optimization
```assembly
; Before optimization
push r0
pop r1       ; If r0 ≠ r1, optimize to mov r1, r0
; Pattern recognition for common sequences
```

## Error Handling & Safety

### Memory Safety
- Bounds checking on all memory accesses
- Stack overflow/underflow detection
- Invalid instruction detection
- Graceful error reporting with context

### Execution Safety
- Instruction limit enforcement
- Controlled execution environment
- Resource cleanup on termination
- Platform-independent error codes

## Performance Characteristics

### When JIT Excels
✅ Mathematical computations  
✅ Loop-heavy algorithms  
✅ Repetitive instruction patterns  
✅ Long-running programs  

### When Interpreter is Preferred
⚠️ Very short programs  
⚠️ Complex control flow  
⚠️ Debugging scenarios  
⚠️ Memory-constrained environments  

### Benchmark Results (Typical)
```
Simple Arithmetic:  ~3-5x speedup with JIT
Loop Operations:    ~5-10x speedup with JIT  
Complex Algorithms: ~2-8x speedup with JIT
```

## Execution Flow

### Interpreter Mode
```
1. Load bytecode into VM memory
2. Initialize registers and stack
3. While not halted and within limits:
   a. Fetch next instruction
   b. Decode operands
   c. Execute instruction
   d. Update program counter
4. Return final state
```

### JIT Mode
```
1. Load and optimize bytecode
2. Initialize JIT context
3. While not halted and within limits:
   a. Check JIT cache for current PC
   b. If cached: execute compiled code
   c. If not cached: compile block and execute
   d. Update execution statistics
4. Show JIT statistics if requested
```

## Extensibility Points

### Adding New Instructions
1. Add to `Inst_Type` enumeration
2. Update instruction table in compiler
3. Implement execution logic in VM
4. Add JIT compilation support
5. Update documentation and tests

### Adding Optimization Passes
1. Create new optimization function
2. Integrate into optimization pipeline
3. Add to appropriate optimization level
4. Write test cases
5. Measure performance impact

### Platform Extensions
1. Add platform-specific code generation
2. Update memory management routines
3. Handle calling convention differences
4. Test on target platform
5. Document platform requirements

---

*Copyright (C) 2026 Bayu Setiawan and Rasya Andrean*
*Architecture Version: 1.0*  
*Last Updated: February 2026*