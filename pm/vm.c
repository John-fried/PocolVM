/* vm.c -- The core module to run bytecode in the virtual machine */

/* Copyright (C) 2026 Bayu Setiawan
   This file is part of Pocol, the Pocol Virtual Machine.
   SPDX-License-Identifier: MIT
*/

#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#include "vm.h"
#include "../common.h"
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>

ST_DATA const char *current_path = NULL;

void pocol_error(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "%s: ", program_invocation_name);
	if (current_path)
		fprintf(stderr, "%s: ", current_path);
	vfprintf(stderr, fmt, ap);
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

	/* is regular file? */
	if (!S_ISREG(st.st_mode)) {
		pocol_error("file format not recognized\n");
		goto error;
	}

	size = st.st_size;
	if (size == 0) {
		errno = ENOEXEC;
		goto error;
	}

	if (size > POCOL_MEMORY_SIZE) {
		pocol_error("size exceeds limit: %ld/%d bytes\n", size, POCOL_MEMORY_SIZE);
		goto error;
	}

	*vm = malloc(sizeof(PocolVM));
	if (!(*vm))
		goto error;

	uint32_t magic_header; /* 4 bit magic header */

	memset((*vm), 0, sizeof(**vm));
	fread((*vm)->memory, 1, size, fp);
	memcpy(&magic_header, (*vm)->memory, sizeof(uint32_t));

	if (magic_header != POCOL_MAGIC) {
		pocol_error("wrong magic number `0x%08X`\n", magic_header);
		goto error;
	}
	fclose(fp);

	/* Set initial valuee */
	(*vm)->halt = 0;
	(*vm)->pc = POCOL_MAGIC_SIZE; /* skip magic_header */
	(*vm)->sp = 0;
	return 0;

error:
	if (vm != NULL) pocol_free_vm(*vm);
	if (fp) fclose(fp);
	if (errno)
		pocol_error("%s\n", strerror(errno));
	return -1;
}

/* Free vm */
void pocol_free_vm(PocolVM *vm)
{
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
			return "ENOUNKNOWN";
	}
}

/********************** Executor ************************/

Err pocol_execute_program(PocolVM *vm, int limit)
{
	while (limit != 0 && !vm->halt) {
		Err err = pocol_execute_inst(vm);
		if (err != ERR_OK) {
			pocol_error("0x%02X: %s (addr: %u)\n", vm->memory[vm->pc], err_as_cstr(err), vm->pc);
			return err;
		}
		if (limit > 0)
			--limit;
	}

	return ERR_OK;
}

#define REG_OP(operand) (operand & 0x07) /* get register index from operand */
#define NEXT (vm->memory[vm->pc++])

/* pocol_fetch64 -- utility to fetch 64 bit value from 8bit next memory */
ST_INLN uint64_t pocol_fetch64(PocolVM *vm)
{
	if (vm->pc + 8 >= POCOL_MEMORY_SIZE) {
		pocol_error("%s\n", err_as_cstr(ERR_ILLEGAL_INST_ACCESS));
		exit(ERR_ILLEGAL_INST_ACCESS);
	}

	uint64_t val;
	memcpy(&val, &vm->memory[vm->pc], sizeof(uint64_t));
	vm->pc += 8;
	return val;
}

ST_INLN uint64_t pocol_fetch_operand(PocolVM *vm, uint8_t type)
{
	if (type == OPR_REG) return vm->registers[REG_OP(NEXT)];
	if (type == OPR_IMM) return pocol_fetch64(vm);
	return 0;
}

Err pocol_execute_inst(PocolVM *vm)
{
	if (vm->pc >= POCOL_MEMORY_SIZE)
		return ERR_ILLEGAL_INST_ACCESS;

	uint8_t op = NEXT;
	uint8_t desc = NEXT; /* take byte descriptor */
	uint8_t op1 = DESC_GET_OP1(desc);
	uint8_t op2 = DESC_GET_OP2(desc);

	switch (op) {
		case INST_HALT:
			vm->halt = 1;
			break;

		case INST_PUSH:
			if (vm->sp >= POCOL_STACK_SIZE) return ERR_STACK_OVERFLOW;
			vm->stack[vm->sp++] = pocol_fetch_operand(vm, op1);
			break;

		case INST_POP:
			if (vm->sp == 0) return ERR_STACK_UNDERFLOW;
			vm->registers[REG_OP(NEXT)] = vm->stack[--vm->sp];
			break;

		case INST_ADD: {
			uint64_t *dest = &vm->registers[REG_OP(NEXT)];
			uint64_t src = pocol_fetch_operand(vm, op2); /* fetch next operand */
			*dest += src;
		} break;

		case INST_JMP:
			vm->pc = pocol_fetch_operand(vm, op1);
			break;

		case INST_PRINT: /* (for debugging) */
			printf("%" PRIu64 "", pocol_fetch_operand(vm, op1));
			break;

		default:
			return ERR_ILLEGAL_INST;
	}

	return ERR_OK;
}
