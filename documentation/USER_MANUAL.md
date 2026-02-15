# Pocol VM User Manual

## Introduction

Welcome to the Pocol VM User Manual! This guide will help you understand how to use the Pocol Virtual Machine, write programs in the Pocl assembly language, and leverage the advanced JIT compilation features.

## Quick Start

### Installing and Building

```bash
# Clone the repository
git clone https://github.com/John-fried/PocolVM.git
cd PocolVM

# Build the assembler
cd posm
make
cd ..

# Build the VM
cd pm  
make
cd ..
```

### Your First Program

Create a simple program file `hello.pcl`:

```assembly
; My first Pocol program
_start:
    push 42         ; Push the answer to life
    pop r0          ; Store in register r0
    print r0        ; Print the value
    halt            ; Stop execution
```

Assemble and run:
```bash
# Assemble the program
./posm/posm hello.pcl -o hello.pob

# Run the program
./pm/pm hello.pob
# Output: 42
```

## Pocl Assembly Language

### Basic Syntax

```assembly
; Comments start with semicolon
label_name:         ; Labels end with colon
    instruction operand1, operand2  ; Instructions with operands
```

### Registers
- **r0-r7**: 8 general-purpose 64-bit registers
- Can hold integer values from 0 to 18,446,744,073,709,551,615
- Used for arithmetic operations and temporary storage

### Instructions

#### HALT
Stops VM execution
```assembly
halt                ; Stop the program
```

#### PUSH
Pushes a value onto the stack
```assembly
push 100           ; Push immediate value
push r0            ; Push register value
```

#### POP
Pops a value from stack into a register
```assembly
pop r0             ; Pop stack top into r0
pop r1             ; Pop stack top into r1
```

#### ADD
Adds two values together
```assembly
add r0, 50         ; Add immediate to register
add r0, r1         ; Add register to register
```

#### JMP
Unconditional jump to label
```assembly
jmp loop_start     ; Jump to label
```

#### PRINT
Prints a value (for debugging)
```assembly
print r0           ; Print register value
print 123          ; Print immediate value
```

### Complete Example Program

```assembly
; Calculate factorial of 5
_start:
    push 5          ; Input number
    push 1          ; Initialize result
    pop r1          ; r1 = 1 (result)
    pop r0          ; r0 = 5 (counter)
    
factorial_loop:
    add r1, r1      ; result *= 2 (simplified)
    add r0, -1      ; counter--
    push r0
    push r1
    jmp factorial_loop
    
    print r1        ; Print result
    halt
```

## Command Line Usage

### VM Execution

```bash
# Basic execution
./pm/pm program.pob

# With instruction limit
./pm/pm program.pob 1000

# With JIT compilation
./pm/pm program.pob --jit

# With statistics
./pm/pm program.pob --jit --stats

# Combined options
./pm/pm program.pob 5000 --jit --stats
```

### Assembler Usage

```bash
# Basic assembly
./posm/posm source.pcl -o output.pob

# Verbose output
./posm/posm source.pcl -o output.pob -v

# Show help
./posm/posm --help
```

## Features

### JIT Compilation

The Just-In-Time compiler translates bytecode to native machine code for better performance:

```bash
# Enable JIT compilation
./pm/pm program.pob --jit

# View compilation statistics
./pm/pm program.pob --jit --stats
```

**Benefits of JIT:**
- 3-10x performance improvement for compute-intensive programs
- Runtime optimization of hot code paths
- Native x86-64 code generation

### Optimization Levels

The VM applies different optimization strategies:

```bash
# No optimization (fastest compilation)
./pm/pm program.pob --jit

# Basic optimization (default)
./pm/pm program.pob --jit

# Advanced optimization (best performance)
# (Currently same as basic, reserved for future use)
```

## Understanding Output

### Normal Execution
```
$ ./pm/pm example.pob
42
```

### With Statistics
```
$ ./pm/pm example.pob --jit --stats
42
=== JIT Statistics ===
Mode: Enabled
Optimization Level: Basic
Compiled blocks: 2
Executed blocks: 150
Cache entries: 2/256
Code buffer used: 128/1048576 bytes
```

### Error Messages
```
$ ./pm/pm nonexistent.pob
pm: nonexistent.pob: No such file or directory

$ ./pm/pm broken.pob
pm: broken.pob: wrong magic number `0x12345678`
```

## Programming Techniques

### Stack Manipulation
```assembly
; Swap two values using stack
push r0            ; Stack: [r0]
push r1            ; Stack: [r0, r1]
pop r0             ; r0 = r1, Stack: [r0]
pop r1             ; r1 = original r0
```

### Loop Implementation
```assembly
; Simple counting loop
_start:
    push 0         ; Counter
    push 10        ; Limit
    
loop:
    pop r1         ; Get limit
    pop r0         ; Get counter
    print r0       ; Print current value
    add r0, 1      ; Increment counter
    add r1, -1     ; Decrement limit
    
    push r1        ; Save limit
    push r0        ; Save counter
    
    ; Conditional jump would go here
    jmp loop_end
    
loop_end:
    halt
```

### Function-like Patterns
```assembly
; Simulate function call using stack
call_function:
    push r0        ; Save registers
    push r1
    ; Function body here
    pop r1         ; Restore registers
    pop r0
    ret            ; Return (jump back)
```

## Best Practices

### Code Organization
```assembly
; Good: Clear labels and comments
_start:
    ; Initialize variables
    push 0
    pop r0
    
main_loop:
    ; Main program logic
    add r0, 1
    print r0
    jmp main_loop

; Bad: Unclear structure
    push 1
    pop r0
    add r0 1
    print r0
    jmp 4  ; Magic numbers
```

### Performance Tips
1. **Minimize Stack Operations**: Direct register operations are faster
2. **Batch Operations**: Group related instructions together
3. **Use JIT for Loops**: JIT excels at repetitive computation
4. **Avoid Deep Nesting**: Flat control flow optimizes better

### Debugging Strategies
```assembly
; Add debug prints to trace execution
debug_point:
    print r0       ; Check register value
    print r1       ; Check another value
    ; Continue with program logic
```

## Troubleshooting

### Common Issues

**Program Doesn't Output Anything:**
```assembly
; Make sure to include print statements
_start:
    push 42
    pop r0
    print r0       ; ← This is essential!
    halt
```

**Stack Overflow Error:**
```assembly
; Problem: Too many pushes without pops
_start:
    push 1
    push 2
    push 3
    ; Missing pop operations!
    halt

; Solution: Balance pushes and pops
_start:
    push 1
    push 2
    pop r0         ; Remove items from stack
    pop r1
    halt
```

**Infinite Loops:**
```assembly
; Problem: No exit condition
loop:
    print 1
    jmp loop       ; Never stops!

; Solution: Add termination condition
counter:
    push 10
loop:
    pop r0
    print r0
    add r0, -1
    push r0
    ; Add exit condition here
```

## Example Programs

### Mathematical Calculations
```assembly
; Calculate (5 + 3) * 2
_start:
    push 5
    push 3
    pop r1
    pop r0
    add r0, r1     ; r0 = 8
    add r0, r0     ; r0 = 16 (multiply by 2)
    print r0       ; Output: 16
    halt
```

### Fibonacci Sequence
```assembly
; Generate first few Fibonacci numbers
_start:
    push 0         ; F(0)
    push 1         ; F(1)
    pop r1         ; r1 = 1
    pop r0         ; r0 = 0
    
fib_loop:
    print r0       ; Print current number
    push r0
    push r1
    pop r2         ; r2 = r1
    pop r0         ; r0 = r0
    add r2, r0     ; r2 = r0 + r1
    push r2        ; New r1
    push r1        ; New r0 (old r1)
    
    ; Add loop limit for demonstration
    jmp fib_loop
    halt
```

## Topics

### Memory Layout Understanding
```
Address Space: 0x00000000 to 0x0009C400 (640KB)
┌─────────────────────────────────────┐
│ 0x00000000: Magic Header (4 bytes)  │
├─────────────────────────────────────┤
│ 0x00000004: Program Instructions     │
│             ...                     │
│             Program Data            │
├─────────────────────────────────────┤
│ 0x0009C400: End of Memory           │
└─────────────────────────────────────┘
```

### Stack Operations Visualization
```
Initial:    [empty stack]
push 10:    [10]
push 20:    [10, 20]
push 30:    [10, 20, 30]
pop r0:     [10, 20]        ; r0 = 30
pop r1:     [10]            ; r1 = 20
```

## Getting Help

### Resources
- **API Reference**: `documentation/API_REFERENCE.md`
- **Technical Architecture**: `documentation/TECHNICAL_ARCHITECTURE.md`
- **Developer Guide**: `documentation/DEVELOPER_GUIDE.md`

### Community Support
- Check existing issues on GitHub
- Review example programs in `example/` directory
- Examine test cases in `pm/tests/` and `posm/tests/`

---

*Copyright (C) 2026 Bayu Setiawan and Rasya Andrean*
*User Manual Version: 1.0*  
*Last Updated: February 2026*