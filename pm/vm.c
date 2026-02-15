/* vm.c -- The core module to run bytecode in the virtual machine */

/* Copyright (C) 2026 Bayu Setiawan and Rasya Andrean
   This file is part of Pocol, the Pocol Virtual Machine.
   SPDX-License-Identifier: MIT
*/

#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#include "vm.h"
#include "jit.h"
#include "vm_syscalls.h"
#include "../common.h"
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#define fileno _fileno
#define fstat _fstat
#define S_ISREG(x) (((x) & _S_IFMT) == _S_IFREG)
#ifndef errno
extern int errno;
#endif
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

ST_DATA const char *current_path = NULL;

void pocol_error(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
#ifdef _GNU_SOURCE
	fprintf(stderr, "%s: ", program_invocation_name);
#else
	fprintf(stderr, "pm: ");
#endif
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
	FILE *fp = fopen(path, "rb");
	if (!fp)
		goto error;

	if (fstat(fileno(fp), &st) < 0)
		goto error;

	PocolHeader header;
	if (fread(&header, sizeof(PocolHeader), 1, fp) != 1) {
		pocol_error("unsupported file format\n");
		goto error;
	}

	if (header.magic != POCOL_MAGIC) {
		pocol_error("wrong magic number `0x%08X`\n", magic_header);
		goto error;
	}

	if (header.version != POCOL_VERSION) {
		pocol_error("program version not supported (expected %d, got %d)\n", POCOL_VERSION,
			header.version);
		goto error;
	}

	if (header.code_size > POCOL_MEMORY_SIZE) {
		pocol_error("size exceeds limit: %ld/%d bytes\n", size, POCOL_MEMORY_SIZE);
		goto error;
	}

	*vm = malloc(sizeof(PocolVM));
	if (!(*vm))
		goto error;

	memset((*vm), 0, sizeof(**vm));
	fread((*vm)->memory, 1, st.st_size, fp);

	/* Initialize JIT context if available */
	(*vm)->jit_context = NULL;

	/* Initialize system call context */
	(*vm)->syscall_ctx = malloc(sizeof(SysCallContext));
	if ((*vm)->syscall_ctx) {
		syscalls_init((*vm)->syscall_ctx);
	}

	fclose(fp);

	/* Set initial valuee */
	(*vm)->halt = 0;
	(*vm)->pc = header.entry_point; /* skip magic_header */
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
	if (vm->jit_context) {
		pocol_jit_free((JitContext*)vm->jit_context);
		free(vm->jit_context);
	}

	/* Free system call context */
	if (vm->syscall_ctx) {
		syscalls_free(vm->syscall_ctx);
		free(vm->syscall_ctx);
	}

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

Err pocol_execute_program_jit(PocolVM *vm, int limit, int jit_enabled)
{
	if (jit_enabled) {
		/* Initialize JIT context if not already done */
		if (!vm->jit_context) {
			vm->jit_context = malloc(sizeof(JitContext));
			if (!vm->jit_context) {
				pocol_error("Failed to allocate JIT context\n");
				return ERR_ILLEGAL_INST_ACCESS;
			}
			pocol_jit_init((JitContext*)vm->jit_context, JIT_MODE_ENABLED, OPT_LEVEL_BASIC);
		}

		/* Apply optimizations */
		Err opt_err = pocol_optimize_bytecode(vm, OPT_LEVEL_BASIC);
		if (opt_err != ERR_OK) {
			pocol_error("Optimization failed: %s\n", err_as_cstr(opt_err));
			return opt_err;
		}

		/* Execute with JIT */
		return pocol_jit_execute_program((JitContext*)vm->jit_context, vm, limit);
	} else {
		/* Use interpreter */
		return pocol_execute_program(vm, limit);
	}
}
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

		case INST_SYS: {
			/* System call: r0 = syscall number, r1-r4 = arguments */
			int syscall_num = (int)vm->registers[0];

			if (vm->syscall_ctx) {
				syscalls_exec(vm->syscall_ctx, vm, syscall_num);
			} else {
				vm->registers[0] = -1;  /* Syscall not available */
			}
			break;
		}

		default:
			return ERR_ILLEGAL_INST;
	}

	return ERR_OK;
}

/* Initialize system call context */
void pocol_syscall_init(PocolVM *vm) {
	if (!vm->syscall_ctx) {
		vm->syscall_ctx = malloc(sizeof(SysCallContext));
	}
	if (vm->syscall_ctx) {
		syscalls_init(vm->syscall_ctx);
	}
}

/* Free system call context */
void pocol_syscall_free(PocolVM *vm) {
	if (vm->syscall_ctx) {
		syscalls_free(vm->syscall_ctx);
		free(vm->syscall_ctx);
		vm->syscall_ctx = NULL;
	}
}
