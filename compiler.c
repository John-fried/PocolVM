#include "common.h"
#include "pocolvm.h"
#include "compiler.h"

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
    [INST_PRINT] = { .type = INST_PRINT, .name = "print", .operand = 1, },
};

/* -- global variable -- */
ST_DATA char *source_file;
ST_DATA char *source;
ST_DATA char *cursor = NULL;
ST_DATA unsigned int line = 1;
ST_DATA unsigned int col = 1;
ST_DATA unsigned int error_count = 0;

/* forward declaration*/
ST_FUNC void consume(void);

/*------------------------------------------------*/

ST_FUNC void compiler_error(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "\x1b[1m%s:%d:%d: \x1b[31merror:\x1b[0m ", source_file, line, col);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	fflush(stderr);
	error_count++;

	if (cursor)
		/* skip until newline (one line, one error -- prevent garbage error from one line) */
		while (*cursor != '\n' && *cursor != '\0') consume();
}

/********************** Lexer *************************/

/* consume -- move cursor to the next character */
ST_FUNC void consume(void)
{
	if (*cursor == '\0')
		return;

	if (*cursor == '\n') {
		line++;
		col = 1;
	}
	else
		col++;
	cursor++;
}

/* next -- take the next token from cursor */
ST_FUNC Token next(void)
{
	while (*cursor != '\0') {
		if (isspace((unsigned char)*cursor) || *cursor == ',')
			consume();
		/* skip comment until newline */
		else if (*cursor == ';')
			while (*cursor != '\n' && *cursor != '\0') consume();
		else
			break;
	}

	Token t = { .start = cursor, .length = 0, .value = 0, .type = TOK_ILLEGAL};

	if (*cursor == '\0') {
		t.type = TOK_EOF;
		return t;
	}

	/* digit? OR ('-'? AND digit(after '-')? ) */
	if (isdigit((unsigned char)*cursor) || (*cursor == '-' &&
	    isdigit((unsigned char)*(cursor + 1)))) {
		t.type = TOK_INT;
		char *end;
		errno = 0;
		t.value = strtol(cursor, &end, 10);
		t.length = end - cursor;
		if (errno == ERANGE)
			compiler_error("Integrer out of range");
		/* consume integrer */
		for (unsigned int i=0;i < t.length;i++)
			consume();
		return t;
	}

	/* decide whether is identifier or register.
	   check a-Z
	*/
	if (isalpha((unsigned char)*cursor)) {
		char *start = cursor;
		/* check a-Z or 1-10 or '_' */
		while (isalnum((unsigned char)*cursor) || *cursor == '_')
			consume();

		t.length = cursor - start;
		t.type = TOK_IDENT;

		/* check if register */
		if (start[0] == 'r') {
			t.type = TOK_REGISTER;
			if (isdigit(start[1]))
				t.value = atoi(start + 1);
		}

		return t;
	}

	/* no valid token */
	compiler_error("Illegal character '%c' in program", *cursor);
	consume();
	return t;
}

/* peek -- take the next n token from cursor
   without moving the cursor (unlike next())
*/
ST_FUNC Token peek(int n)
{
	/* save "before" data */
	char *saved_cursor = cursor;
	unsigned int saved_line = line;
	unsigned int saved_col = col;

	Token t;

	for (int i = 0; i < (n + 1); i++) {
		t = next();
	}

	cursor = saved_cursor;
	line = saved_line;
	col = saved_col;
	return t;
}

/******************** Pocol Compiler ************************/

ST_FUNC void pocol_write64(FILE *out, uint64_t val)
{
	/* Serialize 64-bit value into 8 bytes using Little-Endian order */
	/* stackoverflow.com/questions/69968061/how-do-i-split-an-unsigned-64-bit-int-into-individual-8-bits-little-endian-in */
	uint8_t bytes[8];
	for (int i = 0; i<8; i++)
		bytes[i] = (val >> (i * 8)) & 0xFF;

	fwrite(bytes, 1, 8, out);
}

/*********************** Parser *****************************/

/* take the next token from cursor and store it into parser lookahead */
ST_FUNC void parser_advance(Parser *p)
{
	p->lookahead = next();
}

/* parse instruction, the compiler core -- using the Instruction Set Architecture (ISA)
   it will do the following thing:
   - search if instruction is in the table,
   - check the operand count && operand types,
   - write the mnemonic opcode && write the operand opcode
*/
ST_FUNC void parse_inst(Parser *p)
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

	if (!inst) {
		compiler_error("unknown `%.*s` instruction in program", mnemonic.length, mnemonic.start);
		parser_advance(p);
		return;
	}

	/* Create operand descriptor */
	OperandType types[2] = {OPR_NONE, OPR_NONE};
	for (int i = 0; i < inst->operand; i++) {
		Token t = peek(i);
		if (t.type == TOK_REGISTER) types[i] = OPR_REG;
		else if (t.type == TOK_INT) types[i] = OPR_IMM;
	}

	/* write opcode & byte descriptor */
	uint8_t opcode = (uint8_t)inst->type;
	uint8_t desc = DESC_PACK(types[0], types[1]);

	parser_advance(p); /* skip mnemonic */
	fwrite(&opcode, 1, 1, p->out);
	fwrite(&desc, 1, 1, p->out); /* write descriptor for operand */

	for (int i = 0; i < inst->operand; i++) {
		uint64_t val = (uint64_t)p->lookahead.value;
		parser_advance(p); /* skip val */

		if (types[i] == OPR_REG) {
			uint8_t reg = (uint8_t)val;
			fwrite(&reg, 1, 1, p->out);
			continue;
		}

		pocol_write64(p->out, val);
	}
}

int pocol_compile_file(char *path, char *out)
{
	struct stat st;
	int fd = open(path, O_RDONLY);
	Parser p;
	source_file = path;

	if (fd < 0) goto error;
	if (fstat(fd, &st) < 0) goto error;

	source = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (source == MAP_FAILED) goto error;

	/* create temp output file */
	char tempfile[1024];
	snprintf(tempfile, sizeof(tempfile), "/tmp/%ju.pob.tmp", (uintmax_t)st.st_ino);

	p.out = fopen(tempfile, "wb");
	if (!p.out)
		goto error;

	/* write magic */
	uint32_t magic_header = POCOL_MAGIC;
	fwrite(&magic_header, sizeof(uint32_t), 1, p.out);

	cursor = source;
	p.lookahead = next();
	/* Compiling process */
	while (p.lookahead.type != TOK_EOF) {
		/* only parse instruction if the token.type is identifier */
		if (p.lookahead.type == TOK_IDENT) {
			parse_inst(&p);
			continue;
		}

		parser_advance(&p); /* next token (if token != identifier) */
	}

	munmap(source, st.st_size);
	close(fd);
	fclose(p.out);

	if (error_count > 0) {
		cursor = NULL; /* prevent <segfault> */
		compiler_error("Compilation failed. (%d total errors)", error_count);
		unlink(tempfile);
		return -1;
	}

	/* move tempfile to cwd output */
	if (rename(tempfile, out) < 0) {
		source_file = tempfile;
		compiler_error("failed to move to `%s`", out);
		unlink(tempfile);
		goto error;
	}

	/* mark executable */
	chmod(out, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP
		| S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
	return 0;

error:
	compiler_error("%s", strerror(errno));
	return -1;
}
