/* vm.h -- The core module to run bytecode in the virtual machine */

/* Copyright (C) 2026 Bayu Setiawan and Rasya Andrean
   This file is part of Pocol, the Pocol Virtual Machine.
   SPDX-License-Identifier: MIT
*/

#ifndef POCOL_POCOLVM_H
#define POCOL_POCOLVM_H

#define POCOL_MAGIC         0x6f636f70  /* 'o' 'c' 'o' 'p' reversed can be seen using 'cat' */
#define POCOL_MAGIC_SIZE    4
#define POCOL_OPERAND_MAX	2
#define POCOL_MEMORY_SIZE	(640 * 1000)
#define POCOL_STACK_SIZE	1024

#include <stdint.h>
#include <stdlib.h>	/* used for size_t and also used for memory management */

typedef enum {
	ERR_OK = 0,
	ERR_ILLEGAL_INST,
	ERR_ILLEGAL_INST_ACCESS,
	ERR_STACK_OVERFLOW,
	ERR_STACK_UNDERFLOW,
} Err;

typedef uint64_t Inst_Addr;
typedef uint64_t Stack_Addr;

#define DESC_PACK(op1, op2) (((op2) << 4) | (op1))  /* pack descriptor operand 1 & 2*/
#define DESC_GET_OP1(desc)  ((desc) & 0x0F)         /* get operand 1; 0x0F: 0000 1111 */
#define DESC_GET_OP2(desc)  ((desc) >> 4)           /* get operand 2*/

typedef enum {
    OPR_NONE = 0,
    OPR_REG = 0x01,    /* Register (r0-r7) */
    OPR_IMM = 0x02,    /* Immediate/Integer (5, 100) */
} OperandType;

typedef enum {
    INST_HALT = 0,
    INST_PUSH,
    INST_POP,
    INST_ADD,
    INST_JMP,
    INST_PRINT,
    INST_SYS,     /* System call instruction */
    COUNT_INST	/* last index, start with 0 (halt) and this counts */
} Inst_Type;

typedef struct {
    Inst_Type type;
    const char *name;
    int operand; /* operand count */
} Inst_Def;

/* Include system calls header */
#include "vm_syscalls.h"

typedef struct {
	/* Basic components */
	uint8_t    memory[POCOL_MEMORY_SIZE];  	/* Memory address Register */
	Inst_Addr  pc; 				/* program counter (64Kb memory, 0-65.535) as the MEMORY_SIZE */
	uint64_t   stack[POCOL_STACK_SIZE]; 	/* stack for operation */
	Stack_Addr sp; 				/* stack pointer (0-255) as the STACK_SIZE and +1 space */
	uint64_t   registers[8]; 		/* 8 registers */
	unsigned int halt : 1;			/* halt status */
	
	/* JIT context (optional) */
	void *jit_context;                      /* Opaque pointer to JIT context */
	
	/* System call context */
	SysCallContext *syscall_ctx;          /* System call context */
} PocolVM;

int pocol_load_program_into_vm(const char *path, PocolVM **vm);
void pocol_free_vm(PocolVM *vm);
Err pocol_execute_program(PocolVM *vm, int limit);
Err pocol_execute_inst(PocolVM *vm);

/* JIT execution functions */
Err pocol_execute_program_jit(PocolVM *vm, int limit, int jit_enabled);

/* System call functions */
void pocol_syscall_init(PocolVM *vm);
void pocol_syscall_free(PocolVM *vm);

void pocol_error(const char *fmt, ...);

#endif /* POCOL_POCOLVM_H */
