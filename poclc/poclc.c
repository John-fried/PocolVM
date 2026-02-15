/* poclc.c -- PocolC High-Level Language Compiler Implementation */

/* Copyright (C) 2026 Bayu Setiawan and Rasya Andrean
   This file is part of Pocol, the Pocol Virtual Machine.
   SPDX-License-Identifier: MIT
*/

#include "poclc.h"
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>

/* Forward declarations */
static int is_at_end(CompilerCtx *ctx);
static char advance(CompilerCtx *ctx);
static char peek(CompilerCtx *ctx);
static char peek_next(CompilerCtx *ctx);

/* Keyword lookup table */
static struct {
    const char *keyword;
    TokenType token;
} keywords[] = {
    {"func", TOK_FUNC},
    {"var", TOK_VAR},
    {"if", TOK_IF},
    {"else", TOK_ELSE},
    {"while", TOK_WHILE},
    {"for", TOK_FOR},
    {"return", TOK_RETURN},
    {"print", TOK_PRINT},
    {"input", TOK_INPUT},
    {NULL, TOK_ERROR}
};

/* Initialize lexer */
void init_lexer(CompilerCtx *ctx, FILE *input, const char *input_path) {
    ctx->input = input;
    ctx->input_path = input_path;
    ctx->output = NULL;
    ctx->output_path = NULL;
    
    ctx->buffer = NULL;
    ctx->buffer_size = 0;
    ctx->buffer_pos = 0;
    ctx->line = 1;
    ctx->column = 1;
    
    ctx->var_count = 0;
    ctx->temp_count = 0;
    ctx->error_count = 0;
    ctx->warning_count = 0;
    
    memset(&ctx->current_token, 0, sizeof(Token));
    memset(&ctx->previous_token, 0, sizeof(Token));
    
    /* Initialize variable table */
    memset(ctx->variables, 0, sizeof(ctx->variables));
}

/* Free lexer resources */
void free_lexer(CompilerCtx *ctx) {
    if (ctx->buffer) {
        free(ctx->buffer);
        ctx->buffer = NULL;
    }
}

/* Check if at end of input */
static int is_at_end(CompilerCtx *ctx) {
    return feof(ctx->input);
}

/* Advance and return current character */
static char advance(CompilerCtx *ctx) {
    if (is_at_end(ctx)) {
        return '\0';
    }
    
    char c = fgetc(ctx->input);
    if (c == '\n') {
        ctx->line++;
        ctx->column = 1;
    } else {
        ctx->column++;
    }
    return c;
}

/* Peek at current character without advancing */
static char peek(CompilerCtx *ctx) {
    if (is_at_end(ctx)) {
        return '\0';
    }
    
    int pos = ftell(ctx->input);
    char c = fgetc(ctx->input);
    fseek(ctx->input, pos, SEEK_SET);
    return c;
}

/* Peek at next character */
static char peek_next(CompilerCtx *ctx) {
    if (is_at_end(ctx)) {
        return '\0';
    }
    
    int pos = ftell(ctx->input);
    char c = fgetc(ctx->input);
    char c2 = fgetc(ctx->input);
    fseek(ctx->input, pos, SEEK_SET);
    return c2;
}

/* Skip whitespace and comments */
static void skip_whitespace(CompilerCtx *ctx) {
    while (!is_at_end(ctx)) {
        char c = peek(ctx);
        
        /* Skip whitespace */
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance(ctx);
            continue;
        }
        
        /* Skip single-line comments */
        if (c == '/' && peek_next(ctx) == '/') {
            while (peek(ctx) != '\n' && !is_at_end(ctx)) {
                advance(ctx);
            }
            continue;
        }
        
        /* Skip multi-line comments */
        if (c == '/' && peek_next(ctx) == '*') {
            advance(ctx);  /* Skip / */
            advance(ctx);  /* Skip * */
            while (!is_at_end(ctx)) {
                if (peek(ctx) == '*' && peek_next(ctx) == '/') {
                    advance(ctx);  /* Skip * */
                    advance(ctx);  /* Skip / */
                    break;
                }
                advance(ctx);
            }
            continue;
        }
        
        break;
    }
}

/* Read identifier or keyword */
static Token read_identifier(CompilerCtx *ctx) {
    Token token;
    token.line = ctx->line;
    token.column = ctx->column;
    token.lexeme = NULL;
    
    int capacity = 16;
    int length = 0;
    char *buffer = malloc(capacity);
    if (!buffer) {
        token.type = TOK_ERROR;
        return token;
    }
    
    while (isalnum(peek(ctx)) || peek(ctx) == '_') {
        if (length + 1 >= capacity) {
            capacity *= 2;
            buffer = realloc(buffer, capacity);
        }
        buffer[length++] = advance(ctx);
    }
    buffer[length] = '\0';
    
    token.lexeme = buffer;
    token.type = TOK_IDENT;
    
    /* Check for keywords */
    for (int i = 0; keywords[i].keyword != NULL; i++) {
        if (strcmp(buffer, keywords[i].keyword) == 0) {
            token.type = keywords[i].token;
            break;
        }
    }
    
    return token;
}

/* Read number literal */
static Token read_number(CompilerCtx *ctx) {
    Token token;
    token.line = ctx->line;
    token.column = ctx->column;
    token.lexeme = NULL;
    token.value = 0;
    
    int capacity = 16;
    int length = 0;
    char *buffer = malloc(capacity);
    if (!buffer) {
        token.type = TOK_ERROR;
        return token;
    }
    
    while (isdigit(peek(ctx))) {
        if (length + 1 >= capacity) {
            capacity *= 2;
            buffer = realloc(buffer, capacity);
        }
        buffer[length++] = advance(ctx);
    }
    buffer[length] = '\0';
    
    token.value = atoi(buffer);
    token.type = TOK_NUMBER;
    free(buffer);
    
    return token;
}

/* Read string literal */
static Token read_string(CompilerCtx *ctx) {
    Token token;
    token.line = ctx->line;
    token.column = ctx->column;
    token.lexeme = NULL;
    token.string = NULL;
    
    advance(ctx);  /* Skip opening quote */
    
    int capacity = 16;
    int length = 0;
    char *buffer = malloc(capacity);
    if (!buffer) {
        token.type = TOK_ERROR;
        return token;
    }
    
    while (peek(ctx) != '"' && !is_at_end(ctx)) {
        if (length + 1 >= capacity) {
            capacity *= 2;
            buffer = realloc(buffer, capacity);
        }
        buffer[length++] = advance(ctx);
    }
    
    if (is_at_end(ctx)) {
        free(buffer);
        token.type = TOK_ERROR;
        return token;
    }
    
    advance(ctx);  /* Skip closing quote */
    buffer[length] = '\0';
    
    token.string = buffer;
    token.type = TOK_STRING;
    
    return token;
}

/* Get next token */
Token next_token(CompilerCtx *ctx) {
    skip_whitespace(ctx);
    
    ctx->previous_token = ctx->current_token;
    
    if (is_at_end(ctx)) {
        ctx->current_token.type = TOK_EOF;
        ctx->current_token.line = ctx->line;
        ctx->current_token.column = ctx->column;
        return ctx->current_token;
    }
    
    char c = advance(ctx);
    
    /* Single character tokens */
    switch (c) {
        case '(': ctx->current_token.type = TOK_LPAREN; break;
        case ')': ctx->current_token.type = TOK_RPAREN; break;
        case '{': ctx->current_token.type = TOK_LBRACE; break;
        case '}': ctx->current_token.type = TOK_RBRACE; break;
        case ',': ctx->current_token.type = TOK_COMMA; break;
        case ';': ctx->current_token.type = TOK_SEMICOLON; break;
        
        case '+': ctx->current_token.type = TOK_PLUS; break;
        case '-': ctx->current_token.type = TOK_MINUS; break;
        case '*': ctx->current_token.type = TOK_MULT; break;
        case '/': ctx->current_token.type = TOK_DIV; break;
        case '%': ctx->current_token.type = TOK_MOD; break;
        
        case '=':
            if (peek(ctx) == '=') {
                advance(ctx);
                ctx->current_token.type = TOK_EQ;
            } else {
                ctx->current_token.type = TOK_ASSIGN;
            }
            break;
            
        case '!':
            if (peek(ctx) == '=') {
                advance(ctx);
                ctx->current_token.type = TOK_NE;
            } else {
                ctx->current_token.type = TOK_ERROR;
            }
            break;
            
        case '<':
            if (peek(ctx) == '=') {
                advance(ctx);
                ctx->current_token.type = TOK_LE;
            } else {
                ctx->current_token.type = TOK_LT;
            }
            break;
            
        case '>':
            if (peek(ctx) == '=') {
                advance(ctx);
                ctx->current_token.type = TOK_GE;
            } else {
                ctx->current_token.type = TOK_GT;
            }
            break;
            
        case '"':
            /* Put character back for string handler */
            fseek(ctx->input, -1, SEEK_CUR);
            ctx->column--;
            return read_string(ctx);
            
        default:
            if (isalpha(c) || c == '_') {
                /* Put character back for identifier handler */
                fseek(ctx->input, -1, SEEK_CUR);
                ctx->column--;
                return read_identifier(ctx);
            }
            
            if (isdigit(c)) {
                /* Put character back for number handler */
                fseek(ctx->input, -1, SEEK_CUR);
                ctx->column--;
                return read_number(ctx);
            }
            
            ctx->current_token.type = TOK_ERROR;
            break;
    }
    
    ctx->current_token.line = ctx->line;
    ctx->current_token.column = ctx->column;
    
    return ctx->current_token;
}

/* Consume current token and advance */
void consume_token(CompilerCtx *ctx) {
    next_token(ctx);
}

/* Check if current token matches type */
bool check_token(CompilerCtx *ctx, TokenType type) {
    return ctx->current_token.type == type;
}

/* Match and consume if token matches */
bool match_token(CompilerCtx *ctx, TokenType type) {
    if (!check_token(ctx, type)) {
        return false;
    }
    next_token(ctx);
    return true;
}

/* Error reporting */
void error_token(CompilerCtx *ctx, const char *message) {
    fprintf(stderr, "Error at line %d, column %d: %s\n",
            ctx->current_token.line, ctx->current_token.column, message);
    ctx->error_count++;
}

/* Compiler error with format */
void compiler_error(CompilerCtx *ctx, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    fprintf(stderr, "Error at line %d, column %d: ",
            ctx->current_token.line, ctx->current_token.column);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    
    va_end(args);
    ctx->error_count++;
}

/* Compiler warning with format */
void compiler_warning(CompilerCtx *ctx, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    fprintf(stderr, "Warning at line %d, column %d: ",
            ctx->current_token.line, ctx->current_token.column);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    
    va_end(args);
    ctx->warning_count++;
}

/* Allocate new AST node */
AstNode *new_ast_node(CompilerCtx *ctx, AstNodeType type) {
    AstNode *node = calloc(1, sizeof(AstNode));
    if (!node) {
        ERROR(ctx, "Memory allocation failed");
        return NULL;
    }
    
    node->type = type;
    node->line = ctx->current_token.line;
    node->child_count = 0;
    node->child_capacity = 4;
    node->children = calloc(4, sizeof(AstNode*));
    
    return node;
}

/* Add child to AST node */
void ast_add_child(AstNode *parent, AstNode *child) {
    if (!parent || !child) return;
    
    if (parent->child_count >= parent->child_capacity) {
        parent->child_capacity *= 2;
        parent->children = realloc(parent->children, 
                                    parent->child_capacity * sizeof(AstNode*));
    }
    
    parent->children[parent->child_count++] = child;
}

/* Free AST node and all children */
void free_ast(AstNode *node) {
    if (!node) return;
    
    /* Free string allocations */
    if (node->str_value) free(node->str_value);
    
    /* Free children */
    for (int i = 0; i < node->child_count; i++) {
        free_ast(node->children[i]);
    }
    if (node->children) free(node->children);
    
    /* Free linked nodes */
    if (node->left) free_ast(node->left);
    if (node->right) free_ast(node->right);
    if (node->middle) free_ast(node->middle);
    
    free(node);
}

/* Expression parsing */
AstNode *parse_expression(CompilerCtx *ctx) {
    return parse_binary_expr(ctx);
}

/* Binary expression parsing (operator precedence) */
AstNode *parse_binary_expr(CompilerCtx *ctx) {
    AstNode *left = parse_unary_expr(ctx);
    
    while (check_token(ctx, TOK_PLUS) || check_token(ctx, TOK_MINUS) ||
           check_token(ctx, TOK_MULT) || check_token(ctx, TOK_DIV) ||
           check_token(ctx, TOK_MOD) || check_token(ctx, TOK_EQ) ||
           check_token(ctx, TOK_NE) || check_token(ctx, TOK_LT) ||
           check_token(ctx, TOK_LE) || check_token(ctx, TOK_GT) ||
           check_token(ctx, TOK_GE)) {
        
        TokenType op = ctx->current_token.type;
        consume_token(ctx);
        
        AstNode *right = parse_unary_expr(ctx);
        
        AstNode *binary = new_ast_node(ctx, AST_BINARY_EXPR);
        binary->left = left;
        binary->right = right;
        /* Store operator in str_value temporarily */
        char op_str[2] = {0};
        op_str[0] = (char)op;
        binary->str_value = strdup(op_str);
        
        left = binary;
    }
    
    return left;
}

/* Unary expression parsing */
AstNode *parse_unary_expr(CompilerCtx *ctx) {
    if (check_token(ctx, TOK_MINUS)) {
        consume_token(ctx);
        AstNode *operand = parse_unary_expr(ctx);
        
        AstNode *unary = new_ast_node(ctx, AST_UNARY_EXPR);
        unary->left = operand;
        unary->str_value = strdup("-");
        return unary;
    }
    
    return parse_primary(ctx);
}

/* Primary expression parsing */
AstNode *parse_primary(CompilerCtx *ctx) {
    /* Number literal */
    if (check_token(ctx, TOK_NUMBER)) {
        AstNode *node = new_ast_node(ctx, AST_NUMBER_EXPR);
        node->num_value = ctx->current_token.value;
        consume_token(ctx);
        return node;
    }
    
    /* String literal */
    if (check_token(ctx, TOK_STRING)) {
        AstNode *node = new_ast_node(ctx, AST_STRING_EXPR);
        node->str_value = strdup(ctx->current_token.string);
        consume_token(ctx);
        return node;
    }
    
    /* Identifier */
    if (check_token(ctx, TOK_IDENT)) {
        AstNode *node = new_ast_node(ctx, AST_IDENT_EXPR);
        node->str_value = strdup(ctx->current_token.lexeme);
        consume_token(ctx);
        return node;
    }
    
    /* Parenthesized expression */
    if (check_token(ctx, TOK_LPAREN)) {
        consume_token(ctx);
        AstNode *expr = parse_expression(ctx);
        
        if (!check_token(ctx, TOK_RPAREN)) {
            error_token(ctx, "Expected ')' after expression");
        }
        consume_token(ctx);
        
        return expr;
    }
    
    /* Function call */
    if (check_token(ctx, TOK_PRINT) || check_token(ctx, TOK_INPUT)) {
        TokenType call_type = ctx->current_token.type;
        consume_token(ctx);
        
        AstNode *node = new_ast_node(ctx, AST_CALL_EXPR);
        
        if (check_token(ctx, TOK_LPAREN)) {
            consume_token(ctx);
            
            if (!check_token(ctx, TOK_RPAREN)) {
                ast_add_child(node, parse_expression(ctx));
                
                while (check_token(ctx, TOK_COMMA)) {
                    consume_token(ctx);
                    ast_add_child(node, parse_expression(ctx));
                }
            }
            
            if (!check_token(ctx, TOK_RPAREN)) {
                error_token(ctx, "Expected ')' after function arguments");
            }
            consume_token(ctx);
        }
        
        node->str_value = strdup(call_type == TOK_PRINT ? "print" : "input");
        return node;
    }
    
    error_token(ctx, "Unexpected token in expression");
    consume_token(ctx);
    return NULL;
}

/* Statement parsing */
AstNode *parse_statement(CompilerCtx *ctx) {
    /* Block statement */
    if (check_token(ctx, TOK_LBRACE)) {
        AstNode *block = new_ast_node(ctx, AST_BLOCK);
        consume_token(ctx);
        
        while (!check_token(ctx, TOK_RBRACE) && !check_token(ctx, TOK_EOF)) {
            ast_add_child(block, parse_statement(ctx));
        }
        
        if (!check_token(ctx, TOK_RBRACE)) {
            error_token(ctx, "Expected '}' after block");
        }
        consume_token(ctx);
        
        return block;
    }
    
    /* Variable declaration */
    if (check_token(ctx, TOK_VAR)) {
        AstNode *decl = new_ast_node(ctx, AST_VAR_DECL);
        consume_token(ctx);
        
        if (check_token(ctx, TOK_IDENT)) {
            decl->str_value = strdup(ctx->current_token.lexeme);
            consume_token(ctx);
            
            if (check_token(ctx, TOK_ASSIGN)) {
                consume_token(ctx);
                decl->left = parse_expression(ctx);
            }
        }
        
        if (!check_token(ctx, TOK_SEMICOLON)) {
            error_token(ctx, "Expected ';' after variable declaration");
        }
        consume_token(ctx);
        
        return decl;
    }
    
    /* Print statement */
    if (check_token(ctx, TOK_PRINT)) {
        AstNode *print_node = new_ast_node(ctx, AST_PRINT_STMT);
        consume_token(ctx);
        
        if (!check_token(ctx, TOK_SEMICOLON)) {
            print_node->left = parse_expression(ctx);
        }
        
        if (!check_token(ctx, TOK_SEMICOLON)) {
            error_token(ctx, "Expected ';' after print statement");
        }
        consume_token(ctx);
        
        return print_node;
    }
    
    /* Return statement */
    if (check_token(ctx, TOK_RETURN)) {
        AstNode *ret = new_ast_node(ctx, AST_RETURN_STMT);
        consume_token(ctx);
        
        if (!check_token(ctx, TOK_SEMICOLON)) {
            ret->left = parse_expression(ctx);
        }
        
        if (!check_token(ctx, TOK_SEMICOLON)) {
            error_token(ctx, "Expected ';' after return statement");
        }
        consume_token(ctx);
        
        return ret;
    }
    
    /* If statement */
    if (check_token(ctx, TOK_IF)) {
        AstNode *if_stmt = new_ast_node(ctx, AST_IF_STMT);
        consume_token(ctx);
        
        /* Condition */
        if (!check_token(ctx, TOK_LPAREN)) {
            error_token(ctx, "Expected '(' after 'if'");
        }
        consume_token(ctx);
        
        if_stmt->left = parse_expression(ctx);
        
        if (!check_token(ctx, TOK_RPAREN)) {
            error_token(ctx, "Expected ')' after if condition");
        }
        consume_token(ctx);
        
        /* Then branch */
        if_stmt->middle = parse_statement(ctx);
        
        /* Else branch */
        if (check_token(ctx, TOK_ELSE)) {
            consume_token(ctx);
            if_stmt->right = parse_statement(ctx);
        }
        
        return if_stmt;
    }
    
    /* While statement */
    if (check_token(ctx, TOK_WHILE)) {
        AstNode *while_stmt = new_ast_node(ctx, AST_WHILE_STMT);
        consume_token(ctx);
        
        if (!check_token(ctx, TOK_LPAREN)) {
            error_token(ctx, "Expected '(' after 'while'");
        }
        consume_token(ctx);
        
        while_stmt->left = parse_expression(ctx);
        
        if (!check_token(ctx, TOK_RPAREN)) {
            error_token(ctx, "Expected ')' after while condition");
        }
        consume_token(ctx);
        
        while_stmt->right = parse_statement(ctx);
        
        return while_stmt;
    }
    
    /* For statement */
    if (check_token(ctx, TOK_FOR)) {
        AstNode *for_stmt = new_ast_node(ctx, AST_FOR_STMT);
        consume_token(ctx);
        
        if (!check_token(ctx, TOK_LPAREN)) {
            error_token(ctx, "Expected '(' after 'for'");
        }
        consume_token(ctx);
        
        /* Init */
        if (!check_token(ctx, TOK_SEMICOLON)) {
            for_stmt->left = parse_expression(ctx);
        }
        consume_token(ctx);
        
        /* Condition */
        if (!check_token(ctx, TOK_SEMICOLON)) {
            for_stmt->middle = parse_expression(ctx);
        }
        consume_token(ctx);
        
        /* Increment */
        if (!check_token(ctx, TOK_RPAREN)) {
            for_stmt->right = parse_expression(ctx);
        }
        consume_token(ctx);
        
        /* Body */
        for_stmt->middle = parse_statement(ctx);
        
        return for_stmt;
    }
    
    /* Expression statement (assignment, function call, etc.) */
    AstNode *expr = parse_expression(ctx);
    
    if (!check_token(ctx, TOK_SEMICOLON)) {
        error_token(ctx, "Expected ';' after expression");
    }
    consume_token(ctx);
    
    return expr;
}

/* Parse function */
AstNode *parse_function(CompilerCtx *ctx) {
    AstNode *func = new_ast_node(ctx, AST_FUNC_DECL);
    
    /* Function name */
    if (check_token(ctx, TOK_IDENT)) {
        func->str_value = strdup(ctx->current_token.lexeme);
        consume_token(ctx);
    } else {
        error_token(ctx, "Expected function name");
        return NULL;
    }
    
    /* Parameters */
    if (check_token(ctx, TOK_LPAREN)) {
        consume_token(ctx);
        
        while (!check_token(ctx, TOK_RPAREN) && !check_token(ctx, TOK_EOF)) {
            if (check_token(ctx, TOK_IDENT)) {
                func->param_count++;
                consume_token(ctx);
                
                if (check_token(ctx, TOK_COMMA)) {
                    consume_token(ctx);
                }
            } else {
                break;
            }
        }
        
        if (!check_token(ctx, TOK_RPAREN)) {
            error_token(ctx, "Expected ')' after function parameters");
        }
        consume_token(ctx);
    }
    
    /* Function body */
    if (check_token(ctx, TOK_LBRACE)) {
        func->left = parse_statement(ctx);
    } else {
        error_token(ctx, "Expected function body");
    }
    
    return func;
}

/* Parse program (list of functions) */
AstNode *parse_program(CompilerCtx *ctx) {
    AstNode *program = new_ast_node(ctx, AST_PROGRAM);
    
    next_token(ctx);  /* Get first token */
    
    while (!check_token(ctx, TOK_EOF)) {
        if (check_token(ctx, TOK_FUNC)) {
            consume_token(ctx);
            ast_add_child(program, parse_function(ctx));
        } else if (check_token(ctx, TOK_VAR)) {
            ast_add_child(program, parse_statement(ctx));
        } else if (check_token(ctx, TOK_SEMICOLON)) {
            consume_token(ctx);  /* Skip empty statements */
        } else {
            error_token(ctx, "Unexpected token at top level");
            consume_token(ctx);
        }
    }
    
    return program;
}

/* ============================================
 * CODE GENERATOR - Generate PocolVM Bytecode
 * ============================================ */

/* PocolVM instruction opcodes */
#define POCOL_MAGIC     0x706f636f  /* "poco" in memory */

/* Emit a byte to output */
static void emit_byte(FILE *out, uint8_t byte) {
    fputc(byte, out);
}

/* Emit a 64-bit value to output */
static void emit_u64(FILE *out, uint64_t value) {
    fwrite(&value, sizeof(uint64_t), 1, out);
}

/* Emit descriptor */
static void emit_desc(FILE *out, uint8_t op1, uint8_t op2) {
    emit_byte(out, (op2 << 4) | op1);
}

/* Register to variable mapping */
typedef struct {
    char name[32];
    int reg_num;
    int in_use;
} RegInfo;

static RegInfo registers[8];
static int next_reg = 0;

/* Initialize registers */
static void init_registers(void) {
    memset(registers, 0, sizeof(registers));
    next_reg = 0;
}

/* Allocate a register */
static int alloc_register(void) {
    for (int i = 0; i < 8; i++) {
        if (!registers[i].in_use) {
            registers[i].in_use = 1;
            return i;
        }
    }
    /* Spill to stack if no registers available */
    return -1;
}

/* Free a register */
static void free_register(int reg) {
    if (reg >= 0 && reg < 8) {
        registers[reg].in_use = 0;
    }
}

/* Generate code for expression */
static void gen_expr(FILE *out, AstNode *expr) {
    if (!expr) return;
    
    switch (expr->type) {
        case AST_NUMBER_EXPR: {
            /* Push immediate to stack */
            emit_byte(out, 0x01);  /* INST_PUSH */
            emit_desc(out, 0x02, 0);  /* OPR_IMM, OPR_NONE */
            emit_u64(out, expr->num_value);
            break;
        }
        
        case AST_IDENT_EXPR: {
            /* Load variable from register */
            emit_byte(out, 0x04);  /* INST_POP */
            emit_desc(out, 0x01, 0);  /* OPR_REG, OPR_NONE */
            /* Variable to register mapping would go here */
            emit_byte(out, 0);  /* r0 */
            break;
        }
        
        case AST_BINARY_EXPR: {
            /* Generate left and right expressions */
            gen_expr(out, expr->left);
            gen_expr(out, expr->right);
            
            /* Perform operation */
            emit_byte(out, 0x03);  /* INST_ADD */
            emit_desc(out, 0x01, 0x01);  /* REG, REG */
            emit_byte(out, 0);  /* r0 */
            emit_byte(out, 1);  /* r1 */
            break;
        }
        
        case AST_UNARY_EXPR: {
            if (expr->str_value && strcmp(expr->str_value, "-") == 0) {
                gen_expr(out, expr->left);
                /* Multiply by -1 */
                emit_byte(out, 0x01);  /* INST_PUSH */
                emit_desc(out, 0x02, 0);
                emit_u64(out, 0xFFFFFFFFFFFFFFFFULL);  /* -1 in two's complement */
                emit_byte(out, 0x03);  /* INST_ADD */
                emit_desc(out, 0x01, 0x01);
                emit_byte(out, 0);
                emit_byte(out, 1);
            }
            break;
        }
        
        case AST_CALL_EXPR: {
            if (expr->str_value && strcmp(expr->str_value, "print") == 0) {
                if (expr->child_count > 0) {
                    gen_expr(out, expr->children[0]);
                }
                emit_byte(out, 0x05);  /* INST_PRINT */
                emit_desc(out, 0x01, 0);  /* OPR_REG */
                emit_byte(out, 0);  /* r0 */
            }
            break;
        }
        
        default:
            break;
    }
}

/* Generate code for statement */
static void gen_stmt(FILE *out, AstNode *stmt) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case AST_BLOCK: {
            for (int i = 0; i < stmt->child_count; i++) {
                gen_stmt(out, stmt->children[i]);
            }
            break;
        }
        
        case AST_VAR_DECL: {
            /* Assign expression to variable */
            if (stmt->left) {
                gen_expr(out, stmt->left);
                /* Store result in variable (would map to register) */
            }
            break;
        }
        
        case AST_ASSIGN: {
            gen_expr(out, stmt->right);
            /* Store in variable */
            break;
        }
        
        case AST_PRINT_STMT: {
            if (stmt->left) {
                gen_expr(out, stmt->left);
                emit_byte(out, 0x05);  /* INST_PRINT */
                emit_desc(out, 0x01, 0);
                emit_byte(out, 0);  /* r0 */
            }
            break;
        }
        
        case AST_RETURN_STMT: {
            if (stmt->left) {
                gen_expr(out, stmt->left);
            }
            emit_byte(out, 0x00);  /* INST_HALT */
            break;
        }
        
        case AST_IF_STMT: {
            /* Generate condition */
            if (stmt->left) {
                gen_expr(out, stmt->left);
            }
            /* Would generate jump here for false condition */
            /* Generate then branch */
            if (stmt->middle) {
                gen_stmt(out, stmt->middle);
            }
            /* Generate else branch */
            if (stmt->right) {
                gen_stmt(out, stmt->right);
            }
            break;
        }
        
        case AST_WHILE_STMT: {
            /* Would generate loop structure here */
            if (stmt->right) {
                gen_stmt(out, stmt->right);
            }
            break;
        }
        
        case AST_FOR_STMT: {
            /* Would generate for loop here */
            break;
        }
        
        default:
            break;
    }
}

/* Generate code for program */
void generate_code(CompilerCtx *ctx, AstNode *ast) {
    if (!ast || !ctx->output) return;
    
    /* Write PocolVM magic header */
    emit_u64(ctx->output, POCOL_MAGIC);
    
    /* Generate code for each function/statement */
    for (int i = 0; i < ast->child_count; i++) {
        gen_stmt(ctx->output, ast->children[i]);
    }
    
    /* Add HALT at the end */
    emit_byte(ctx->output, 0x00);  /* INST_HALT */
}
