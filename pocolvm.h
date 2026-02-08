/* pocolvm.h -- The core module to run bytecode in the virtual machine */

/* Copyright (C) 2026 Bayu Setiawan
   This file is part of Pocol, the Pocol Virtual Machine.
   SPDX-License-Identifier: MIT
*/

#ifndef POCOL_POCOLVM_H
#define POCOL_POCOLVM_H

#define MEMORY_SIZE	65535
#define STACK_SIZE	256

#include <stdint.h>
#include <stdlib.h>	/* used for size_t and also used for memory management */

typedef struct {
	/* Basic components */
	uint8_t  memory[MEMORY_SIZE];  /* Memory address Register */
	uint16_t pc; /* program counter (64Kb memory, 0-65.535) as the MEMORY_SIZE */
	uint32_t stack[STACK_SIZE]; /* stack for operation */
	uint8_t  sp; /* stack pointer (0-255) as the STACK_SIZE and +1 space */
	uint32_t registers[8]; /* 8 registers */
} PocolVM;

PocolVM *pocol_make_vm(uint8_t *bytecode, size_t size);
void pocol_free_vm(PocolVM *vm);
uint8_t pocol_run_vm(PocolVM *vm);

#endif /* POCOL_POCOLVM_H */
