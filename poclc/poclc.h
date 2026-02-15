/* poclc.h -- PocolC High-Level Language Compiler */

/* Copyright (C) 2026 Bayu Setiawan and Rasya Andrean
   This file is part of Pocol, the Pocol Virtual Machine.
   SPDX-License-Identifier: MIT
*/

#ifndef POCOLC_H
#define POCOLC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Token types for PocolC lexer */
typedef enum {
    TOK_EOF = 0,
    TOK_IDENT,
    TOK_NUMBER,
    TOK_STRING,
    
    /* Keywords */
    TOK_FUNC,
    TOK_VAR,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FOR,
    TOK_RETURN,
    TOK_PRINT,
    TOK_INPUT,
    
    /* Operators */
    TOK_PLUS,
    TOK_MINUS,
    TOK_MULT,
    TOK_DIV,
    TOK_MOD,
    TOK_ASSIGN,
    TOK_EQ,
    TOK_NE,
    TOK_LT,
    TOK_LE,
    TOK_GT,
    TOK_GE,
    
    /* Delimiters */
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_COMMA,
    TOK_SEMICOLON,
    
    /* Special */
    TOK_ERROR,
    TOK_COMMENT,
} TokenType;

/* Token structure */
typedef struct {
    TokenType type;
    char *lexeme;
    int value;          /* For numbers */
    char *string;       /* For strings */
    int line;
    int column;
} Token;

/* AST Node Types */
typedef enum {
    AST_PROGRAM,
    AST_FUNC_DECL,
    AST_VAR_DECL,
    AST_ASSIGN,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_FOR_STMT,
    AST_RETURN_STMT,
    AST_PRINT_STMT,
    AST_BINARY_EXPR,
    AST_UNARY_EXPR,
    AST_CALL_EXPR,
    AST_IDENT_EXPR,
    AST_NUMBER_EXPR,
    AST_STRING_EXPR,
    AST_BLOCK,
    AST_PARAM_LIST,
    AST_ARG_LIST,
} AstNodeType;

/* AST Node Structure */
typedef struct AstNode {
    AstNodeType type;
    int line;
    
    /* Value for number expressions */
    int num_value;
    
    /* String value for identifiers and strings */
    char *str_value;
    
    /* Child nodes */
    struct AstNode *left;
    struct AstNode *right;
    struct AstNode *middle;  /* For if-else */
    
    /* Lists */
    struct AstNode **children;
    int child_count;
    int child_capacity;
    
    /* Function info */
    char *func_name;
    int param_count;
} AstNode;

/* Compiler Context */
typedef struct {
    FILE *input;
    FILE *output;
    const char *input_path;
    const char *output_path;
    
    /* Current token */
    Token current_token;
    Token previous_token;
    
    /* Input buffer */
    char *buffer;
    size_t buffer_size;
    size_t buffer_pos;
    int line;
    int column;
    
    /* Symbol table */
    char variables[256][32];
    int var_count;
    int temp_count;
    
    /* Error tracking */
    int error_count;
    int warning_count;
} CompilerCtx;

/* Function prototypes */

/* Lexer functions */
void init_lexer(CompilerCtx *ctx, FILE *input, const char *input_path);
void free_lexer(CompilerCtx *ctx);
Token next_token(CompilerCtx *ctx);
void consume_token(CompilerCtx *ctx);
bool check_token(CompilerCtx *ctx, TokenType type);
bool match_token(CompilerCtx *ctx, TokenType type);
void error_token(CompilerCtx *ctx, const char *message);

/* Parser functions */
AstNode *parse_program(CompilerCtx *ctx);
AstNode *parse_function(CompilerCtx *ctx);
AstNode *parse_statement(CompilerCtx *ctx);
AstNode *parse_expression(CompilerCtx *ctx);
AstNode *parse_binary_expr(CompilerCtx *ctx);
AstNode *parse_unary_expr(CompilerCtx *ctx);
AstNode *parse_primary(CompilerCtx *ctx);
void free_ast(AstNode *node);

/* Code generator */
void generate_code(CompilerCtx *ctx, AstNode *ast);

/* Utility functions */
void compiler_error(CompilerCtx *ctx, const char *fmt, ...);
void compiler_warning(CompilerCtx *ctx, const char *fmt, ...);

/* Helper macros */
#define ERROR(ctx, msg) compiler_error(ctx, msg)
#define WARNING(ctx, msg) compiler_warning(ctx, msg)

#endif /* POCOLC_H */
