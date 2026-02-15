#ifndef POCOL_LEXER_H
#define POCOL_LEXER_H

#include "compiler.h"

/* consume -- move cursor forward (detect newlines, terminator will does nothing) */
void consume(CompilerCtx *ctx);

void consume_until_newline(CompilerCtx *ctx);

/* next -- take the next token from cursor */
Token next(CompilerCtx *ctx);

/* peek -- take the next n token from cursor
   without moving the cursor (unlike next())
*/
Token peek(CompilerCtx *ctx, int n);

#endif /* POCOL_LEXER_H */
