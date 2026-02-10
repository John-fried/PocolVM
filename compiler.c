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

const Inst_Def inst_table[COUNT_INST] = {
    [INST_HALT]  = { .type = INST_HALT,  .name = "halt", .operand = 0 },
    [INST_PUSH]  = { .type = INST_PUSH,  .name = "push", .operand = 1,
    			.operand_type = {OPR_IMM} },
    [INST_POP]   = { .type = INST_POP,   .name = "pop", .operand = 1,
    			.operand_type = {OPR_REG} },
    [INST_ADD]   = { .type = INST_ADD,   .name = "add", .operand = 2,
    			.operand_type = {OPR_REG, OPR_REG} },
    [INST_PRINT] = { .type = INST_PRINT, .name = "print", .operand = 1,
    			.operand_type = {OPR_REG} },
};

/* -- global variable -- */
ST_DATA char *source_file;
ST_DATA char *source;
ST_DATA char *cursor;
ST_DATA unsigned int line = 1;
ST_DATA unsigned int col = 1;
ST_DATA unsigned int error_count = 0;

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
}

/********************** Lexer *************************/

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

ST_FUNC Token next(void)
{
	while (*cursor != '\0') {
		if (isspace((unsigned char)*cursor) || *cursor == ',')
			consume();
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

	if (isdigit((unsigned char)*cursor) || (*cursor == '-' &&
	    isdigit((unsigned char)*(cursor + 1)))) {
		t.type = TOK_INT;
		char *end;
		errno = 0;
		t.value = strtol(cursor, &end, 10);
		t.length = end - cursor;
		if (errno == ERANGE)
			compiler_error("Integrer out of range");
		for (unsigned int i=0;i < t.length;i++)
			consume();
		return t;
	}

	if (isalpha((unsigned char)*cursor)) {
		char *start = cursor;
		while (isalnum((unsigned char)*cursor) || *cursor == '_')
			consume();

		t.length = cursor - start;
		t.type = TOK_IDENT;

		// check if register
		if (start[0] == 'r' && isdigit(start[1])) {
			t.type = TOK_REGISTER;
			t.value = atoi(start + 1);
		}

		return t;
	}

	compiler_error("Illegal character '%c' in program", *cursor);
	consume();
	return t;
}

ST_FUNC Token peek(int n)
{
	// save "before" data
	char *saved_cursor = cursor;
	unsigned int saved_line = line;
	unsigned int saved_col = col;

	Token t;

	for (int i = 0; i < n; i++) {
		t = next();
	}

	cursor = saved_cursor;
	line = saved_line;
	col = saved_col;
	return t;
}

/*********************** Parser *****************************/

ST_FUNC void parser_advance(Parser *p)
{
	p->lookahead = next();
}

ST_FUNC void parse_inst(Parser *p)
{
	unsigned int table = 0;
	Token mnemonic = p->lookahead;
	const Inst_Def *inst = NULL;

search_table:
	// search from instruction table
	for (; table < COUNT_INST; table++) {
		// @check name
		if (strncmp(inst_table[table].name, mnemonic.start, mnemonic.length) == 0 &&
			strlen(inst_table[table].name) == mnemonic.length) {
			inst = &inst_table[table];
			break;
		}
	}

	if (!inst) {
		compiler_error("unknown `%.*s` instruction in program", mnemonic.length, mnemonic.start);
		parser_advance(p);
		return;
	}

	// @validate operand
	int match = 1;
	Token t;

	for (int i = 0; i < inst->operand; i++) {
		t = peek(i + 1);
		if (inst->operand_type[i] == OPR_REG && t.type != TOK_REGISTER) match = 0;
		if (inst->operand_type[i] == OPR_IMM && t.type != TOK_INT) match = 0;
	}

	if (!match) {
		inst = NULL;
		table++;
		goto search_table;
	}

	uint8_t opcode = (uint8_t)inst->type;
	parser_advance(p); // skip mnemonic
	fwrite(&opcode, 1, 1, p->out);

	for (int i = 0; i < inst->operand; i++) {
		uint8_t val = (uint8_t)p->lookahead.value;
		fwrite(&val, 1, 1, p->out);
		parser_advance(p);
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

	p.out = fopen(out, "wb");
	if (!p.out) goto error;

	cursor = source;
	p.lookahead = next();
	/* Compiling process */
	while (p.lookahead.type != TOK_EOF) {
		if (p.lookahead.type == TOK_IDENT) {
			parse_inst(&p);
			continue;
		}

		parser_advance(&p);
	}

	munmap(source, st.st_size);
	close(fd);
	fclose(p.out);

	// mark executable
	chmod(out, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP
		| S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
	return 0;

error:
	compiler_error("%s", strerror(errno));
	return -1;
}
