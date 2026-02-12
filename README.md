# Pocol VM

Pocol is a minimalist, register-based [Virtual Machine](https://en.wikipedia.org/wiki/Virtual_machine) with a stack, written in pure [C](https://en.wikipedia.org/wiki/C_(programming_language)). It includes a compiler to transform `.pcl` assembly-like source into `.pob` (Pocol Binary).

## Architecture

* **Registers**: 8 general-purpose 64-bit registers (`r0`-`r7`).
* **Stack**: 1024 slots for 64-bit integers.
* **Memory**: 640KB linear address space.
* **Word Size**: 64-bit [Little-Endian](https://en.wikipedia.org/wiki/Endianness).

## SubProjects

* [pm](https://github.com/John-fried/PocolVM/tree/main/pm) - VM Core emulator
* [posm](https://github.com/John-fried/PocolVM/tree/main/pm) - Assembler for the VM Bytecode.

## Building

The build system is a simple `Makefile`. If you have `gcc` and `make`, you are good to go.

```bash
make
```

> Build on posm/ or pm/ directories

## License

MIT. See headers for copyright details.
