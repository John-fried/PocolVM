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

ST_DATA const Inst_Def inst_defs[COUNT_INST] = {
    [INST_HALT]  = { .type = INST_HALT,  .name = "halt"  },
    [INST_PUSH]  = { .type = INST_PUSH,  .name = "push"  },
    [INST_POP]   = { .type = INST_POP,   .name = "pop"   },
    [INST_ADD]   = { .type = INST_ADD,   .name = "add"   },
    [INST_PRINT] = { .type = INST_PRINT, .name = "print" },
};

/* make and load bytecode into vm */
PocolVM *pocol_make_vm(uint8_t *bytecode, size_t size)
{
	PocolVM *vm = malloc(sizeof(PocolVM));
	if (!vm)
		return NULL;

	memset(vm->memory, 0, POCOL_MEMORY_SIZE); /* prevent garbage value from other program */
	memset(vm->registers, 0, sizeof(vm->registers)); /* prevent garbage value from other program */
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
	if (!vm)
		return;

	free(vm);
}

#define IS_MEM(operand) operand & 0x80
#define MEM_OP(operand) operand & 0x7F
#define REG_OP(operand) operand & 0x07

/* Register vs Memory */
ST_INLN uint32_t get_val(PocolVM *vm, uint8_t operand)
{
	/* checks up if bit[7] is >1 or 0 */
	if (IS_MEM(operand))
		return vm->memory[MEM_OP(operand)];

	return vm->registers[REG_OP(operand)];
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
			vm->registers[ REG_OP(NEXT) ] = vm->stack[--vm->sp];
			break;

		case INST_ADD: {
			// res = CIR <dest> + PEEK(1) <src>
			uint32_t res = get_val(vm, CIR) + get_val(vm, PEEK(1));
			if (IS_MEM( CIR ))
				vm->memory[ MEM_OP(NEXT) ] = (uint8_t) res;
			else
				vm->registers[ REG_OP(NEXT) ] = res;
			vm->pc++;
		} break;

		case INST_PRINT: /* (for debugging) */
			printf("%d", get_val(vm, NEXT));
			break;

		default:
			return ERR_ILLEGAL_INST;
	}

	return ERR_OK;
}
