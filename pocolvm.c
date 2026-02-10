/* pocolvm.c -- The core module to run bytecode in the virtual machine */

/* Copyright (C) 2026 Bayu Setiawan
   This file is part of Pocol, the Pocol Virtual Machine.
   SPDX-License-Identifier: MIT
*/

#include "pocolvm.h"
#include "common.h"
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

ST_DATA const char *current_path;

ST_FUNC void pocol_error(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "`%s`: vm.error: ", current_path);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	fflush(stderr);
}

/* make and load bytecode into vm */
void pocol_load_program_into_vm(const char *path, PocolVM **vm)
{
	FILE *fp = fopen(path, "rb"); // read binary mode
	if (!fp) return;
	current_path = path;

	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (size <= 0) {
		pocol_error("Program section is empty.");
		goto error;
	}

	if (size > POCOL_MEMORY_SIZE) {
		pocol_error("Program section is too big. This file contains %ld instructions. But the capacity is %d",
			size, POCOL_MEMORY_SIZE);
		goto error;
	}

	*vm = malloc(sizeof(PocolVM));
	if (!(*vm)) {
		pocol_error("%s", strerror(errno));
		fclose(fp);
		return;
	}

	memset(*vm, 0, sizeof(**vm));
	fread((*vm)->memory, 1, size, fp);
	fclose(fp);

	/* Set initial valuee */
	(*vm)->halt = 0;
	(*vm)->pc = 0;
	(*vm)->sp = 0;
	return;

error:
	fprintf(stderr, "vm.load: failed\n");
	fclose(fp);
}

/* Free vm */
void pocol_free_vm(PocolVM *vm)
{
	assert(vm != NULL);
	free(vm);
}

ST_FUNC char *err_as_cstr(Err err) {
	switch (err) {
		case ERR_OK:
			return "OK";
		case ERR_STACK_OVERFLOW:
			return "stack overflow";
		case ERR_STACK_UNDERFLOW:
			return "stack underflow";
		case ERR_ILLEGAL_INST:
			return "unrecognized opcode";
		case ERR_ILLEGAL_INST_ACCESS:
			return "illegal memory access";
		default:
			return "???";
	}
}

Err pocol_execute_program(PocolVM *vm, int limit)
{
	while (limit != 0 && !vm->halt) {
		Err err = pocol_execute_inst(vm);
		if (err != ERR_OK) {
			pocol_error("%d: %s", vm->memory[vm->pc], err_as_cstr(err));
			return err;
		}
		if (limit > 0)
			--limit;
	}

	return ERR_OK;
}

Err pocol_execute_inst(PocolVM *vm)
{
	if (vm->pc >= POCOL_MEMORY_SIZE)
		return ERR_ILLEGAL_INST_ACCESS;

	#define REG_OP(operand) (operand & 0x07)
	#define NEXT (vm->memory[vm->pc++])

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
