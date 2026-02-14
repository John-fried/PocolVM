#include "lexer.h"
#include <errno.h>
#include <ctype.h>

/* consume -- move cursor to the next character */
void consume(CompilerCtx *ctx)
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

void consume_until_newline(CompilerCtx *ctx)
{
	if (ctx->cursor)
		while (*ctx->cursor != '\n' && *ctx->cursor != '\0') consume(ctx);
}

/* next -- take the next token from cursor */
Token next(CompilerCtx *ctx)
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
Token peek(CompilerCtx *ctx, int n)
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
