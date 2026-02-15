/* Copyright (C) 2026 Bayu Setiawan and Rasya Andrean
   This file is part of Pocol, the Pocol Virtual Machine.
   SPDX-License-Identifier: MIT
*/

#ifndef POCOL_COMPILER_H
#define POCOL_COMPILER_H

#include <stdint.h>
#include <stdio.h>

#include "symbol.h"

typedef enum {
	TOK_EOF = 0,
	TOK_ILLEGAL,
	TOK_INT,
	TOK_LABEL, /* ':' */
	TOK_IDENT, /* identifier */
	TOK_REGISTER, /* register (prefix: 'r') */
} TokenType;

typedef struct {
	TokenType type;
	const char *start;
	unsigned int length;
	int64_t value; /* if tok == TOK_INT or TOK_REGISTER r[digit] */
} Token;

typedef struct {
	FILE *out;
	Token lookahead; /* Curent parsed token */
	char *path; /* Current source path */
	char *source; /* source files */
	char *cursor; /* currrent cursor to source files */
	unsigned int line;
	unsigned int col;
	unsigned int total_error;
	PocolSymbol symbols; /* Compiler symbol table */
} CompilerCtx;

int pocol_compile_file(CompilerCtx *ctx, char *out);
void compiler_error(CompilerCtx *ctx , const char *fmt, ...);

#endif /* POCOL_COMPILER_H */
