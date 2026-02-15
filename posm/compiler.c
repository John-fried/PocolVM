#define _GNU_SOURCE
#include "../common.h"
#include "../pm/vm.h"
#include "compiler.h"
#include "lexer.h"
#include "emit.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>

#ifdef _WIN32
#include <io.h>
#define unlink _unlink
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

/* instruction table */
const Inst_Def inst_table[COUNT_INST] = {
    [INST_HALT]  = { .type = INST_HALT,  .name = "halt", .operand = 0 },
    [INST_PUSH]  = { .type = INST_PUSH,  .name = "push", .operand = 1, },
    [INST_POP]   = { .type = INST_POP,   .name = "pop", .operand = 1, },
    [INST_ADD]   = { .type = INST_ADD,   .name = "add", .operand = 2, },
    [INST_JMP]   = { .type = INST_JMP,   .name = "jmp", .operand = 1, },
    [INST_PRINT] = { .type = INST_PRINT, .name = "print", .operand = 1, },
};

void compiler_error(CompilerCtx *ctx, const char *fmt, ...)
{
	#ifdef _GNU_SOURCE
		if (ctx->path == NULL) ctx->path = program_invocation_name;
	#else
		if (ctx->path == NULL) ctx->path = "posm";
	#endif

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

/*********************** Parser *****************************/

/* take the next token from cursor and store it into parser lookahead */
ST_INLN void parser_advance(CompilerCtx *ctx)
{
	ctx->lookahead = next(ctx);
}

/* parse instruction -- Instruction Set Architecture (ISA)
   - search if instruction is in the table,
   - check the operand count && operand types,
   - emit mnemonic opcode && emit operand opcode (only if pass 2)

   -1 Instruction not found
*/
ST_FUNC int parse_inst(CompilerCtx *ctx)
{
	Token mnemonic = ctx->lookahead;
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
		Token t = peek(ctx, i);
		if (t.type == TOK_REGISTER) types[i] = OPR_REG;
		else if (t.type == TOK_INT || t.type == TOK_IDENT) types[i] = OPR_IMM;
	}

	/* write opcode & byte descriptor (only if pass 2) */
	if (ctx->pass == 2) {
		uint8_t opcode = (uint8_t)inst->type;
		uint8_t desc = DESC_PACK(types[0], types[1]);

		fwrite(&opcode, 1, 1, ctx->out);
		fwrite(&desc, 1, 1, ctx->out); /* write descriptor for operand */
	}

	ctx->virtual_pc += 2; /* opcode +1, descriptor +1 */
	parser_advance(ctx); /* skip mnemonic */

	for (int i = 0; i < inst->operand; i++) {
		uint64_t val = (uint64_t)ctx->lookahead.value;

		if (ctx->lookahead.type == TOK_IDENT) {
			char name[ctx->lookahead.length + 1];
			memcpy(name, ctx->lookahead.start, ctx->lookahead.length);
			name[ctx->lookahead.length] = '\0';

			SymData *s = pocol_symfind(&ctx->symbols, SYM_LABEL, name);
			if (s)
				/* set as label IP (also as immediate value)  */
				val = s->as.label.pc;
			else if (ctx->pass == 2)
				/* pass 1 shouldn't error (undefined label or something) */
				compiler_error(ctx, "indentifier `%s` not defined", name);
		}

		if (types[i] == OPR_REG) {
			if (ctx->pass == 2) {
				uint8_t reg = (uint8_t)val;
				fwrite(&reg, 1, 1, ctx->out);
			}
			ctx->virtual_pc++; /* 1 byte */
		} else {
			/* use 64 bit for immediate value */
			ctx->virtual_pc += sizeof(uint64_t);
			if (ctx->pass == 2)
				emit64(ctx->out, val);
		}

		parser_advance(ctx); /* skip to next operand */
	}

	return 0;
}

ST_FUNC void pocol_parse_file(CompilerCtx *ctx)
{
	while (ctx->lookahead.type != TOK_EOF) {
		if (ctx->pass == 1 && ctx->lookahead.type == TOK_LABEL) {
			/* push to symbol table (only if pass 1) */
			SymData symdata;

			symdata.kind = SYM_LABEL;
			memcpy(symdata.name, ctx->lookahead.start, ctx->lookahead.length); /* copy label name with start and length */
			symdata.name[ctx->lookahead.length] = '\0';
			symdata.as.label.pc = ctx->virtual_pc; /* use the virtual cursor position as label pc */
			symdata.as.label.is_defined = 1;

			if (pocol_sympush(&ctx->symbols, &symdata) == -1)
				compiler_error(ctx, "duplicate label `%s`", symdata.name);

			/* skip label */
			consume_until_newline(ctx);
			parser_advance(ctx);
			continue;
		}

		if (ctx->lookahead.type == TOK_IDENT) {
			char name[ctx->lookahead.length + 1];
			memcpy(name, ctx->lookahead.start, ctx->lookahead.length);
			name[ctx->lookahead.length] = '\0';

			/* parse instruction */
			if (parse_inst(ctx) == 0) continue;
			compiler_error(ctx, "unknown `%s` instruction in program", name);
			parser_advance(ctx);
			continue;
		}

		parser_advance(ctx); /* next token (skip invalid token) */
	}
}

// NOTE: Don't auto quit if error, just continue. (like gcc/c compiler behavior)
int pocol_compile_file(CompilerCtx *ctx, char *out)
{
	struct stat st;
	int fd = open(ctx->path, O_RDONLY);

	if (fd < 0) goto error;
	if (fstat(fd, &st) < 0) goto error;

	ctx->source = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (ctx->source == MAP_FAILED) goto error;

	/* create temp output file */
	char tempfile[1024];
	snprintf(tempfile, sizeof(tempfile), "/tmp/%ju.pob.tmp", (uintmax_t)st.st_ino);

	ctx->out = fopen(tempfile, "wb");
	if (!ctx->out)
		goto error;

	/* init header */
	PocolHeader header = {0};
	header.magic = POCOL_MAGIC;
	header.version = POCOL_VERSION;
	fwrite(&header, sizeof(PocolHeader), 1, ctx->out); /* create header placeholder. Because after that we emit instruction*/

	/* start parsing */
	for (ctx->pass = 1; ctx->pass <= 2; ctx->pass++) {
		ctx->line = 1;
		ctx->col = 1;
		ctx->virtual_pc = sizeof(PocolHeader); /* reset (skip header offset) */
		ctx->cursor = ctx->source; /* RESET cursor to the beggining of source */
		ctx->lookahead = next(ctx);

		pocol_parse_file(ctx);

		if (ctx->pass == 1 && ctx->total_error > 0)
			break;
	}

	/* program entry point */
	header.code_size = ftell(ctx->out) - sizeof(PocolHeader);
	SymData *start_node = pocol_symfind(&ctx->symbols, SYM_LABEL, "_start");
	if (start_node != NULL)
		header.entry_point = start_node->as.label.pc;
	else
		compiler_error(ctx, "undefined reference to `_start`");


	/* set cursor to header placeholder and overwrite it */
	fseek(ctx->out, 0, SEEK_SET); /* goto beggining of binary stream cursor */
	fwrite(&header, sizeof(PocolHeader), 1, ctx->out);

	munmap(ctx->source, st.st_size);
	close(fd);
	fclose(ctx->out);

	ctx->cursor = NULL;  /* dont skip until newline, we doesnt have anymore string here (EOF) */
	if (ctx->total_error > 0) {
		ctx->line = 0;	/* dont print line:col */

		compiler_error(ctx, "compilation failed. (%d total errors)", ctx->total_error);
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
	if (ctx->source) munmap(ctx->source, st.st_size);
	if (fd > 0) close(fd);
	if (ctx->out) fclose(ctx->out);
	compiler_error(ctx, "%s", strerror(errno));
	return -1;
}
