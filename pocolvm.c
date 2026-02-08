/* pocolvm.c -- The core module to run bytecode in the virtual machine */

/* Copyright (C) 2026 Bayu Setiawan
   This file is part of Pocol, the Pocol Virtual Machine.
   SPDX-License-Identifier: MIT
*/

#include "pocolvm.h"
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* make and load bytecode into vm */
PocolVM *pocol_make_vm(uint8_t *bytecode, size_t size)
{
	PocolVM *vm = malloc(sizeof(PocolVM));
	if (!vm)
		return NULL;

	memset(vm->memory, 0, MEMORY_SIZE); /* prevent garbage value from other program */
	memset(vm->registers, 0, sizeof(vm->registers)); /* prevent garbage value from other program */
	if (bytecode != NULL && size <= MEMORY_SIZE)
		memcpy(vm->memory, bytecode, size);

	/* Set initial valuee */
	vm->pc = 0;
	vm->sp = 0;

	return vm;
}

/* Free vm */
void pocol_free_vm(PocolVM *vm)
{
	if (!vm)
		return;

	free(vm);
}

/* --- Core execution --- */

#define IS_MEM(operand) operand & 0x80

/* helper for getting value between Register vs Memory */
ST_INLN uint32_t get_val(PocolVM *vm, uint8_t operand)
{
	/* checks up if bit[7] is >1 or 0 */
	if (IS_MEM(operand))
		return vm->memory[operand & 0x7F];

	return vm->registers[operand & 0x07];
}

/* Using computed goto for fast instruction
 * Warning!: this non-standard may cause warning when compiling with -Wpedantic,
 */
void pocol_run_vm(PocolVM *vm)
{
	ST_DATA void *dispatch_table[] = {
		&&do_halt, &&do_push, &&do_pop, &&do_add, &&do_print
	};

	#define NEXT() goto *dispatch_table[vm->memory[vm->pc++]]
	NEXT();	/* take first instruction */

	do_halt:
		exit(get_val(vm, vm->memory[vm->pc++]));
	do_push:	/* push <val> */
		vm->stack[vm->sp++] = vm->memory[vm->pc++];
		NEXT();
	do_pop:		/* pop <reg> */
		vm->registers[vm->memory[vm->pc++] & 0x07] = vm->stack[--vm->sp];
		NEXT();
	do_add: {	/* add <dest>, <src> */
		uint8_t dest_info = vm->memory[vm->pc++];
		uint8_t src_info = vm->memory[vm->pc++];

		uint32_t val1 = get_val(vm, dest_info);
		uint32_t val2 = get_val(vm, src_info);
		uint32_t result = val1 + val2;
		if (IS_MEM(dest_info))
			vm->memory[dest_info & 0x07] = result;
		else
			vm->registers[dest_info & 0x7F] = result;
		NEXT();
	}
	do_print:	/* print <src> (debugging) */
		printf("%d\n", get_val(vm, vm->memory[vm->pc++]));
		NEXT();
}
