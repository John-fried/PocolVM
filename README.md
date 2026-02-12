# Pocol VM

Pocol is a minimalist, register-based [Virtual Machine](https://en.wikipedia.org/wiki/Virtual_machine) with a stack, written in pure [C](https://en.wikipedia.org/wiki/C_(programming_language)). It includes a basic compiler to transform `.pcl` assembly-like source into `.pob` (Pocol Binary).

## Architecture

* **Registers**: 8 general-purpose 64-bit registers (`r0`-`r7`).
* **Stack**: 1024 slots for 64-bit integers.
* **Memory**: 640KB linear address space.
* **Word Size**: 64-bit [Little-Endian](https://en.wikipedia.org/wiki/Endianness).

## Building

The build system is a simple `Makefile`. If you have `gcc` and `make`, you are good to go.

```bash
make

```

## Usage

Pocol handles both compilation and execution through a single binary.

### 1. Compiling Source

To compile a `.pcl` file into bytecode (`out.pob`):

```bash
./pocol compile example/3010.pcl

```

### 2. Running Bytecode

To execute the generated binary:

```bash
./pocol out.pob

```

## Instruction Set Architecture (ISA)

The current instruction set is small and functional. Every instruction is followed by an operand descriptor byte.

| Mnemonic | Operands | Description |
| --- | --- | --- |
| `halt` | 0 | Stops execution. |
| `push` | 1 | Pushes a register value or immediate to the stack. |
| `pop` | 1 | Pops value from stack into a register. |
| `add` | 2 | Adds two values and stores result in the first operand (register). |
| `print` | 1 | Prints the value of a register or immediate to `stdout`. |

> And more (if any) im lazy to write here

## Project Structure

* `pocolvm.c/h`: The core [Interpreter](https://en.wikipedia.org/wiki/Interpreter_(computing)) and execution logic.
* `compiler.c/h`: A single-pass [Lexer](https://en.wikipedia.org/wiki/Lexical_analysis) and [Parser](https://en.wikipedia.org/wiki/Parsing).
* `common.h`: Shared macros and build-time configurations.
* `main.c`: CLI entry point.

## License

MIT. See headers for copyright details.
