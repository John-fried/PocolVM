/* pocolvm.c -- The core module to run bytecode in the virtual machine */

/* Copyright (C) 2026 Bayu Setiawan
   This file is part of Pocol, the Pocol Virtual Machine.
   SPDX-License-Identifier: MIT
*/

#include "pocolvm.h"
#include "common.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* make and load bytecode into vm */
PocolVM *pocol_make_vm(uint8_t *bytecode, size_t size)
{
	PocolVM *vm = malloc(sizeof(PocolVM));
	if (!vm)
		return NULL;

	memset(vm, 0, sizeof(*vm));
	if (bytecode != NULL && size <= POCOL_MEMORY_SIZE)
		memcpy(vm->memory, bytecode, size);

	/* Set initial valuee */
	vm->halt = 0;
	vm->pc = 0;
	vm->sp = 0;

	return vm;
}

/* Free vm */
void pocol_free_vm(PocolVM *vm)
{
	assert(vm != NULL);
	free(vm);
}

Err pocol_run_program(PocolVM *vm, int limit)
{
	while (limit != 0 && !vm->halt) {
		Err err = pocol_run_inst(vm);
		if (err != ERR_OK)
			return err;
		if (limit > 0)
			--limit;
	}

	return ERR_OK;
}

Err pocol_run_inst(PocolVM *vm)
{
	if (vm->pc >= POCOL_MEMORY_SIZE)
		return ERR_ILLEGAL_INST_ACCESS;

	#define REG_OP(operand) (operand & 0x07)

	#define CIR      (vm->memory[vm->pc]) /* CIR Value */
	#define PEEK(n)  (vm->memory[vm->pc + (n)]) /* take CIR(n) without incrementing CIR */
	#define NEXT     (vm->memory[vm->pc++])	/* take CIR then next CIR */
	#define NNEXT(n) (vm->memory[vm->pc += (n)]) /* take CIR(n) */

	uint8_t op = NEXT;
	switch (op) {
		case INST_HALT:
			vm->halt = 1;
			break;

		case INST_PUSH:
			if (vm->sp >= POCOL_STACK_SIZE) return ERR_STACK_OVERFLOW;
			vm->stack[vm->sp++] = NEXT;
			break;

		case INST_POP:
			if (vm->sp == 0) return ERR_STACK_UNDERFLOW;
			vm->registers[REG_OP(NEXT)] = vm->stack[--vm->sp];
			break;

		case INST_ADD: {
			uint32_t dest = REG_OP(NEXT);
			uint32_t src = REG_OP(NEXT);
			vm->registers[dest] += vm->registers[src];
		} break;

		case INST_PRINT: /* (for debugging) */
			printf("%d", vm->registers[REG_OP(NEXT)]);
			break;

		default:
			return ERR_ILLEGAL_INST;
	}

	return ERR_OK;
}
