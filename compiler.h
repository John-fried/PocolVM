/* Copyright (C) 2026 Bayu Setiawan
   This file is part of Pocol, the Pocol Virtual Machine.
   SPDX-License-Identifier: MIT
*/

#ifndef POCOL_COMPILER_H
#define POCOL_COMPILER_H

#include <stdint.h>

typedef enum {
	TOK_EOF = 0,
	TOK_INT,
	TOK_COMMA,
	TOK_ERROR,
	TOK_COMMENT,
	TOK_IDENT, // identifier
	TOK_REGISTER, // register (prefix: 'r')
} TokenType;

typedef struct {
	TokenType type;
	const char *start;
	unsigned int length;
	int32_t value; // if tok == TOK_INT
} Token;

#endif // POCOL_COMPILER_H
