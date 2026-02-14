#define _GNU_SOURCE
#include "../common.h"
#include "../pm/vm.h"
#include "compiler.h"
#include "emit.h"

#include <ctype.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

/* instruction table */
const Inst_Def inst_table[COUNT_INST] = {
    [INST_HALT]  = { .type = INST_HALT,  .name = "halt", .operand = 0 },
    [INST_PUSH]  = { .type = INST_PUSH,  .name = "push", .operand = 1, },
    [INST_POP]   = { .type = INST_POP,   .name = "pop", .operand = 1, },
    [INST_ADD]   = { .type = INST_ADD,   .name = "add", .operand = 2, },
    [INST_JMP]   = { .type = INST_JMP,   .name = "jmp", .operand = 1, },
    [INST_PRINT] = { .type = INST_PRINT, .name = "print", .operand = 1, },
};

/* Forward declarations -- for static function */
ST_FUNC void consume(CompilerCtx *ctx);
ST_FUNC void consume_until_newline(CompilerCtx *ctx);

/*------------------------------------------------*/

void compiler_error(CompilerCtx *ctx, const char *fmt, ...)
{
	if (ctx->path == NULL) ctx->path = program_invocation_name;

	va_list ap;
	va_start(ap, fmt);

	/* Print prefix */
	fprintf(stderr, "\x1b[1m%s:", ctx->path);
	if (ctx->line > 0)
		fprintf(stderr, "%d:%d:", ctx->line, ctx->col);
	fprintf(stderr, " \x1b[31merror\x1b[0m: ");

	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	fflush(stderr);
	ctx->total_error++;

	consume_until_newline(ctx);
}

/********************** Lexer *************************/

/* consume -- move cursor to the next character */
ST_FUNC void consume(CompilerCtx *ctx)
{
	if (*ctx->cursor == '\0')
		return;

	if (*ctx->cursor == '\n') {
		ctx->line++;
		ctx->col = 1;
	}
	else
		ctx->col++;
	ctx->cursor++;
}

ST_INLN void consume_until_newline(CompilerCtx *ctx)
{
	if (ctx->cursor)
		while (*ctx->cursor != '\n' && *ctx->cursor != '\0') consume(ctx);
}

/* next -- take the next token from cursor */
ST_FUNC Token next(CompilerCtx *ctx)
{
	while (*ctx->cursor != '\0') {
		if (isspace((unsigned char)*ctx->cursor) || *ctx->cursor == ',')
			consume(ctx);
		/* skip comment until newline */
		else if (*ctx->cursor == ';')
			consume_until_newline(ctx);
		else
			break;
	}

	Token t = { .start = ctx->cursor, .length = 0, .value = 0, .type = TOK_ILLEGAL};

	if (*ctx->cursor == '\0') {
		t.type = TOK_EOF;
		return t;
	}

	/* digit? OR ('-'? AND digit(after '-')? ) */
	if (isdigit((unsigned char)*ctx->cursor) || (*ctx->cursor == '-' &&
	    isdigit((unsigned char)*(ctx->cursor + 1)))) {
		t.type = TOK_INT;
		char *end;
		errno = 0;
		t.value = strtol(ctx->cursor, &end, 10);
		t.length = end - ctx->cursor;
		if (errno == ERANGE)
			compiler_error(ctx, "Integrer out of range");
		/* consume integrer */
		for (unsigned int i=0;i < t.length;i++)
			consume(ctx);
		return t;
	}

	/* decide whether is identifier or register.
	   check a-Z
	*/
	if (isalpha((unsigned char)*ctx->cursor) || *ctx->cursor == '_') {
		/* check a-Z or 1-10 or '_' */
		while (isalnum((unsigned char)*ctx->cursor) || *ctx->cursor == '_')
			consume(ctx);

		/* check if label */
		if (*ctx->cursor == ':') {
			t.type = TOK_LABEL;
			t.length = ctx->cursor - t.start;
			consume(ctx); /* skip ':' */
			return t;
		}

		/* check if register */
		if (t.start[0] == 'r') {
			t.type = TOK_REGISTER;
			if (isdigit(t.start[1])) /* r0 - r..[digit].. */
				t.value = atoi(t.start + 1);
			return t;
		}

		t.length = ctx->cursor - t.start;
		t.type = TOK_IDENT;

		return t;
	}

	/* no valid token */
	compiler_error(ctx, "Illegal character '%c' in program", *ctx->cursor);
	consume(ctx);
	return t;
}

/* peek -- take the next n token from cursor
   without moving the cursor (unlike next())
*/
ST_FUNC Token peek(CompilerCtx *ctx, int n)
{
	/* save "before" data */
	char *saved_cursor = ctx->cursor;
	unsigned int saved_line = ctx->line;
	unsigned int saved_col = ctx->col;

	Token t;

	for (int i = 0; i < (n + 1); i++) {
		t = next(ctx);
	}

	ctx->cursor = saved_cursor;
	ctx->line = saved_line;
	ctx->col = saved_col;
	return t;
}

/*********************** Parser *****************************/

/* take the next token from cursor and store it into parser lookahead */
ST_FUNC void parser_advance(CompilerCtx *ctx, Parser *p)
{
	p->lookahead = next(ctx);
}

/* parse instruction -- Instruction Set Architecture (ISA)
   - search if instruction is in the table,
   - check the operand count && operand types,
   - emit mnemonic opcode && emit operand opcode

   -1 Instruction not found
*/
ST_FUNC int parse_inst(CompilerCtx *ctx, Parser *p)
{
	Token mnemonic = p->lookahead;
	const Inst_Def *inst = NULL;

	/* search from instruction table */
	for (int i = 0; i < COUNT_INST; i++) {
		/* @check name */
		if (strncmp(inst_table[i].name, mnemonic.start, mnemonic.length) == 0 &&
			strlen(inst_table[i].name) == mnemonic.length) {
			inst = &inst_table[i];
			break;
		}
	}

	if (!inst)
		return -1;

	/* Create operand descriptor */
	OperandType types[2] = {OPR_NONE, OPR_NONE};
	for (int i = 0; i < inst->operand; i++) {
		Token t = peek(ctx, 1);
		if (t.type == TOK_REGISTER) types[i] = OPR_REG;
		else if (t.type == TOK_INT || t.type == TOK_IDENT) types[i] = OPR_IMM;
	}

	/* write opcode & byte descriptor */
	uint8_t opcode = (uint8_t)inst->type;
	uint8_t desc = DESC_PACK(types[0], types[1]);

	parser_advance(ctx, p); /* skip mnemonic */
	fwrite(&opcode, 1, 1, p->out);
	fwrite(&desc, 1, 1, p->out); /* write descriptor for operand */

	for (int i = 0; i < inst->operand; i++) {
		uint64_t val = (uint64_t)p->lookahead.value;

		if (p->lookahead.type == TOK_IDENT) {
			char name[p->lookahead.length + 1];
			memcpy(name, p->lookahead.start, p->lookahead.length);
			name[p->lookahead.length] = '\0';

			SymData *s = pocol_symfind(&ctx->symbols, SYM_LABEL, name);
			if (s) val = s->as.label.pc;
			else compiler_error(ctx, "indentifier `%s` not defined", name);
		}

		if (types[i] == OPR_REG) {
			uint8_t reg = (uint8_t)val;
			fwrite(&reg, 1, 1, p->out);
		} else {
			emit64(p->out, val);
		}

		parser_advance(ctx, p); /* skip val */
	}

	return 0;
}

// NOTE: Don't auto quit if error, just continue. (like gcc/c compiler behavior)
int pocol_compile_file(CompilerCtx *ctx, char *out)
{
	struct stat st;
	int fd = open(ctx->path, O_RDONLY);
	Parser p;

	if (fd < 0) goto error;
	if (fstat(fd, &st) < 0) goto error;

	ctx->source = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (ctx->source == MAP_FAILED) goto error;

	/* create temp output file */
	char tempfile[1024];
	snprintf(tempfile, sizeof(tempfile), "/tmp/%ju.pob.tmp", (uintmax_t)st.st_ino);

	p.out = fopen(tempfile, "wb");
	if (!p.out)
		goto error;

	/* write magic */
	uint32_t magic_header = POCOL_MAGIC;
	fwrite(&magic_header, sizeof(uint32_t), 1, p.out);

	/* Set Starter */
	ctx->cursor = ctx->source;
	ctx->parser = &p;
	ctx->line = 1; /* enable line:col prefix if error occured */
	p.lookahead = next(ctx);


	while (p.lookahead.type != TOK_EOF) {
		if (p.lookahead.type == TOK_LABEL) {
			/* push to symbol table */
			SymData symdata;

			symdata.kind = SYM_LABEL;
			memcpy(symdata.name, p.lookahead.start, p.lookahead.length); /* copy label name with start and length */
			symdata.name[p.lookahead.length] = '\0';
			symdata.as.label.pc = (Inst_Addr) ftell(p.out); /* mark this pc */
			symdata.as.label.is_defined = 1;

			if (pocol_sympush(&ctx->symbols, &symdata) == -1)
				compiler_error(ctx, "duplicate label `%s`", symdata.name);

			/* skip label */
			consume_until_newline(ctx);
			parser_advance(ctx, &p);
			continue;
		}

		/* only parse instruction if the token.type is identifier */
		if (p.lookahead.type == TOK_IDENT) {
			char name[p.lookahead.length + 1];
			memcpy(name, p.lookahead.start, p.lookahead.length);
			name[p.lookahead.length] = '\0';

			if (parse_inst(ctx, &p) == 0) continue;

			SymData *label_data = pocol_symfind(&ctx->symbols, SYM_LABEL, name);
			if (label_data != NULL) {
				emit64(p.out, label_data->as.label.pc);
				parser_advance(ctx, &p);
				continue;
			}

			compiler_error(ctx, "unknown `%s` instruction in program", name);

			parser_advance(ctx, &p);
			continue;
		}

		parser_advance(ctx, &p); /* next token (skip invalid token) */
	}

	munmap(ctx->source, st.st_size);
	close(fd);
	fclose(p.out);

	if (ctx->total_error > 0) {
		ctx->cursor = NULL;  /* dont skip until newline, we doesnt have anymore string here (EOF) */
		ctx->line = 0;	/* dont print line:col */

		compiler_error(ctx, "Compilation failed. (%d total errors)", ctx->total_error);
		unlink(tempfile);
		return -1;
	}

	/* move tempfile to cwd output */
	if (rename(tempfile, out) < 0) {
		ctx->path = tempfile;
		compiler_error(ctx, "failed to move to `%s`", out);
		unlink(tempfile);
		goto error;
	}

	/* mark executable */
	chmod(out, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP
		| S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
	return 0;

error:
	compiler_error(ctx, "%s", strerror(errno));
	return -1;
}
