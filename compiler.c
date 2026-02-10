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

/* Example: "add r0, 5"

Conflict:
  [name] [opcode]  [operand type]
  "add": INST_ADD  {reg, reg}
  "add": INST_ADDI {reg, imm}

table = 0;

@check name: gets "add" INST_ADD
@validate operand:
	"add r0, 5"

	operand: r0 PASSED
	operand: 5  FAIL (imm)

	jump invalid operand:
		table = 4 (example)
		table++; (now table = 5)
		jump @check name 2

@check name 2: gets "add" INST_ADD (table 6)
@validate operand:
	"add r0, 5"

	operand: r0 PASSED
	operand: 5  PASSED

	input_reg = r0;
	input_imm = 5;

[ ... WRITE OPCODE LOGIC ...]
*/

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
		compiler_error("`%.*s`: unknown instruction in program", mnemonic.length, mnemonic.start);
		parser_advance(p);
		return;
	}

	uint8_t opcode = (uint8_t)inst->type;
	uint8_t input_reg;
	uint32_t input_imm;

	// @validate operand
	parser_advance(p); // skip mnemonic (example: "add")
	for (int i = 0; i < inst->operand; i++) {
		if (inst->operand_type[i] == OPR_REG) {
			if (p->lookahead.type != TOK_REGISTER)
				goto invalid_operand;

			input_reg = (uint8_t)p->lookahead.value;
			parser_advance(p);
		} else if (inst->operand_type[i] == OPR_IMM) {
			if (p->lookahead.type != TOK_INT)
				goto invalid_operand;

			input_imm = (uint32_t)p->lookahead.value;
			parser_advance(p);
		}
	}

	// emit bytecode
	fwrite(&opcode, 1, 1, p->out);
	if (inst->operand > 0) {
		fwrite(&input_reg, 1, 1, p->out);
		fwrite(&input_imm, sizeof(uint32_t), 1, p->out);
	}

	return;

invalid_operand:
	inst = NULL;
	table++;
	goto search_table;
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
	return 0;

error:
	compiler_error("%s", strerror(errno));
	return -1;
}
