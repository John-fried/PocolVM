/* pocolvm.h -- The core module to run bytecode in the virtual machine */

/* Copyright (C) 2026 Bayu Setiawan
   This file is part of Pocol, the Pocol Virtual Machine.
   SPDX-License-Identifier: MIT
*/

#ifndef POCOL_POCOLVM_H
#define POCOL_POCOLVM_H

#define POCOL_OPERAND_MAX	2
#define POCOL_MEMORY_SIZE	(512 * 100)
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

typedef uint32_t Inst_Addr;
typedef uint32_t Stack_Addr;

typedef enum {
    OPR_NONE = 0,
    OPR_REG,    /* Register (r0-r7) */
    OPR_IMM,    /* Immediate/Integer (5, 100) */
} OperandType;

typedef enum {
    INST_HALT = 0,
    INST_PUSH,
    INST_POP,
    INST_ADD,
    INST_PRINT,
    COUNT_INST	/* last index, start with 0 (halt) and this counts */
} Inst_Type;

typedef struct {
    Inst_Type type;
    const char *name;
    int operand;
    OperandType operand_type[POCOL_OPERAND_MAX];
} Inst_Def;

typedef struct {
	/* Basic components */
	uint8_t    memory[POCOL_MEMORY_SIZE];  	/* Memory address Register */
	Inst_Addr  pc; 				/* program counter (64Kb memory, 0-65.535) as the MEMORY_SIZE */
	uint32_t   stack[POCOL_STACK_SIZE]; 	/* stack for operation */
	Stack_Addr sp; 				/* stack pointer (0-255) as the STACK_SIZE and +1 space */
	uint32_t   registers[8]; 		/* 8 registers */
	unsigned int halt : 1;			/* halt status */
} PocolVM;

void pocol_load_program_into_vm(const char *path, PocolVM **vm);
void pocol_free_vm(PocolVM *vm);
Err pocol_execute_program(PocolVM *vm, int limit);
Err pocol_execute_inst(PocolVM *vm);

#endif /* POCOL_POCOLVM_H */
