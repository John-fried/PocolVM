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
ST_DATA int line = 1;
ST_DATA int col = 0;

/*------------------------------------------------*/

ST_FUNC void compiler_error(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "%s:%d:%d: \x1b[31merror\x1b[0m: ", source_file, line, col);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	fflush(stderr);
}

ST_FUNC void advance(void)
{
	if (*cursor == '\0')
		return;

	if (*cursor == '\n')
		line++;
	else
		col++;
	cursor++;
}

ST_INLN int isnumeric(char *s)
{
	if (*s == '\0') return 0;
	if (*s == '-' && isdigit(*(s + 1))) s += 2;

	while (*s)
		if (!isdigit(*s++)) return 0;

	return 1;
}

Token next(void)
{
	while (*cursor != '\0' && isspace((unsigned char)*cursor))
		advance();

	Token t;
	t.type = TOK_ERROR;
	t.start = cursor;

	if (*cursor == '\0')
		return (Token){TOK_EOF, cursor, 0, 0};

	if (*cursor == ',')
		return (Token){TOK_COMMA, cursor, 0, 1};

	if (isnumeric(cursor)) {
		t.type = TOK_INT;
		t.value = atoi(cursor);
	}

	if (*cursor == ';')
		t.type = TOK_COMMENT;

	if (isalpha(*cursor)) {
		while (isalnum(*cursor))
			advance();
		t.length = (int)(cursor - t.start);
		t.type = TOK_IDENT;
		// r0,r1,r2....
		if (t.start[0] == 'r' && isdigit(t.start[1])) {
			t.type = TOK_REGISTER;
			t.value = t.start[1] - '0';
		}
	}

	return t;
}

// TODO: Refractor this into more modular function or more cleaner logic
// NOTE: use ST_INLN (static inline) for optimizing speed if you want to seperate the logic into smaller function
int compile_file(char *path, char *out)
{
	source_file = path;
	int fd = open(path, O_RDONLY);

	if (fd < 0)
		goto error;

	struct stat st;
	if (fstat(fd, &st) < 0)
		goto error;

	source = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (source == MAP_FAILED)
		goto error;

	/* output file */
	FILE *outfd = fopen(out, "wb"); // write binary mode

	if (!outfd) {
		compiler_error("can't open %s", out);
		goto cleanup;
	}

	/* compile progress */
	Token mnemonic = next();
	Token operands[POCOL_OPERAND_MAX];
	int op_count = 0;

next_instruction:
	while (mnemonic.type != TOK_EOF) {
		if (mnemonic.type != TOK_IDENT) {
			mnemonic = next();
			continue;
		}

		// take operand
		while (op_count < POCOL_OPERAND_MAX) {
			Token t = next();
			if (t.type == TOK_COMMA) t = next(); // skip comma
			if (t.type == TOK_COMMENT) {
				while (*cursor != '\n')
					advance();
				t = next();
			}

			if (t.type == TOK_REGISTER || t.type == TOK_INT)
				operands[op_count++] = t;
			else
				break; // no more operands
		}

		// search for instruction in the table
		const Inst_Def *curr_table;
		for (int i = 0; i < COUNT_INST; i++) {
			curr_table = &inst_table[i];
			if (strncmp(curr_table->name, mnemonic.start, mnemonic.length) == 0 &&
				strlen(curr_table->name) == mnemonic.length &&
				op_count == curr_table->operand) {
				// check operands types
				int match = 1;
				OperandType expected;
				for (int j = 0; j < op_count; j++) {
					expected = curr_table->operand_type[j];
					if (expected == OPR_REG && mnemonic.type != TOK_REGISTER) match = 0;
					if (expected == OPR_IMM && mnemonic.type != TOK_INT) match = 0;
				}

				if (match) {
					uint8_t opcode = (uint8_t )curr_table->type;
					fwrite(&opcode, 1, 1, outfd);

					for (int j = 0; j < op_count; j++) {
						opcode = (uint8_t)operands[j].value;
						fwrite(&opcode, 1, 1, outfd);
					}
					op_count = 0;
					goto next_instruction;
				}
			}
		}

		compiler_error("instruction '%s' not found in instruction table");
	}

cleanup:
	munmap(source, st.st_size);
	close(fd);
	if (outfd) fclose(outfd);
	return 0;

error:
	compiler_error("%s", strerror(errno));
	return -1;
}
