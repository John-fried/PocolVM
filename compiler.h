/* Copyright (C) 2026 Bayu Setiawan
   This file is part of Pocol, the Pocol Virtual Machine.
   SPDX-License-Identifier: MIT
*/

#ifndef POCOL_COMPILER_H
#define POCOL_COMPILER_H

#include <stdint.h>
#include <stdio.h>

typedef enum {
	TOK_EOF = 0,
	TOK_INT,
	TOK_ILLEGAL,
	TOK_IDENT, // identifier
	TOK_REGISTER, // register (prefix: 'r')
} TokenType;

typedef struct {
	TokenType type;
	const char *start;
	unsigned int length;
	int32_t value; // if tok == TOK_INT
} Token;

typedef struct {
	Token lookahead;
	FILE *out;
} Parser;

int pocol_compile_file(char *path, char *out);

#endif // POCOL_COMPILER_H
