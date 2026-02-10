/* pocolvm.c -- The core module to run bytecode in the virtual machine */

/* Copyright (C) 2026 Bayu Setiawan
   This file is part of Pocol, the Pocol Virtual Machine.
   SPDX-License-Identifier: MIT
*/

#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#include "pocolvm.h"
#include "common.h"
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>

ST_DATA const char *current_path;
ST_DATA unsigned int tap = 0;

ST_FUNC void pocol_error(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	if (access(current_path, F_OK) == 0 && !tap) {
		tap++;
		fprintf(stderr, "in file `%s`:\n", current_path);
	}

	fprintf(stderr, "vm.error: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	fflush(stderr);
}

/* make and load bytecode into vm */
int pocol_load_program_into_vm(const char *path, PocolVM **vm)
{
	current_path = path;
	errno = 0;

	struct stat st;
	long size = 0;
	FILE *fp = fopen(path, "rb");
	if (!fp)
		goto error;

	if (fstat(fileno(fp), &st) < 0)
		goto error;

	size = st.st_size;
	if (size == 0) {
		pocol_error("no program to load");
		goto error;
	}

	if (size > POCOL_MEMORY_SIZE) {
		pocol_error("size exceeds limit: %ld/%d bytes", size, POCOL_MEMORY_SIZE);
		goto error;
	}

	*vm = malloc(sizeof(PocolVM));
	if (!(*vm))
		goto error;

	memset((*vm), 0, sizeof(**vm));
	fread((*vm)->memory, 1, size, fp);
	fclose(fp);

	/* Set initial valuee */
	(*vm)->halt = 0;
	(*vm)->pc = 0;
	(*vm)->sp = 0;
	return 0;

error:
	if (errno)
		pocol_error("%s", strerror(errno));

	fprintf(stderr, "%s: error: load failed with %d\n", program_invocation_name, errno);
	if (fp) fclose(fp);
	return -1;
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
			pocol_error("0x%02X: %s (addr: %u)", vm->memory[vm->pc], err_as_cstr(err), vm->pc);
			return err;
		}
		if (limit > 0)
			--limit;
	}

	return ERR_OK;
}

ST_INLN uint64_t pocol_fetch64(PocolVM *vm)
{
	if (vm->pc + 8 >= POCOL_MEMORY_SIZE) {
		pocol_error("fetch-failed: %s", err_as_cstr(ERR_ILLEGAL_INST_ACCESS));
		exit(ERR_ILLEGAL_INST_ACCESS);
	}

	uint64_t val;
	memcpy(&val, &vm->memory[vm->pc], sizeof(uint64_t));
	vm->pc += 8;
	return val;
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
			vm->stack[vm->sp++] = pocol_fetch64(vm);
			break;

		case INST_POP:
			if (vm->sp == 0) return ERR_STACK_UNDERFLOW;
			vm->registers[REG_OP(NEXT)] = vm->stack[--vm->sp];
			break;

		case INST_ADD: {
			uint8_t dest = REG_OP(NEXT);
			uint8_t src = REG_OP(NEXT);
			vm->registers[dest] += vm->registers[src];
		} break;

		case INST_PRINT: /* (for debugging) */
			printf("%" PRIu64 "", vm->registers[REG_OP(NEXT)]);
			break;

		default:
			return ERR_ILLEGAL_INST;
	}

	return ERR_OK;
}
