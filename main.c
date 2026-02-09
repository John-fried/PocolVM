/* main.c -- The entry point of Pocol */

/* Copyright (C) 2026 Bayu Setiawan
   This file is part of Pocol, the Pocol Virtual Machine.
   SPDX-License-Identifier: MIT
*/

#include <stdio.h>
#include <stdint.h>
#include "pocolvm.h"

int main(void)
{
	/* * PocolVM Test Program:
	 * 1. Push 10 and 20 to stack
	 * 2. Pop them into R0 and R1
	 * 3. Add R0 + R1, result in R0
	 * 4. Print R0
	 */
	uint8_t program[] = {
		INST_PUSH, 10,	/* Stack: [10] */
		INST_PUSH, 20,	/* Stack: [10, 20] */
		INST_POP, 0,	/* R0 = 20, Stack: [10] */
		INST_POP, 1,	/* R1 = 10, Stack: [] */
		INST_ADD, 0, 1,	/* R0 = R0 + R1 (30) */
		INST_PRINT, 0,	/* Output R0 */
		INST_HALT	/* Stop execution */
	};

	/* Initialize VM and load bytecode */
	PocolVM *vm = pocol_make_vm(program, sizeof(program));
	if (!vm) {
		fprintf(stderr, "Error: Failed create VM\n");
		return 1;
	}

	printf("Expected: 30\nResult: ");
	Err err = pocol_run_program(vm, -1);	/* run with no limit (-1) */
	putchar('\n');	/* explicit newline */

	if (err != ERR_OK) {
		fprintf(stderr, "\nVM Execution Error: %d\n", err);
	}
	pocol_free_vm(vm);
	return (int)err;
}
