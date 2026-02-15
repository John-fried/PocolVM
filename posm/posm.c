#include "compiler.h"

int main(int argc, char **argv)
{
	char *output = (argc > 2) ? argv[2] : "out.pob";

	CompilerCtx ctx = {
		.path = NULL,
		.source = NULL,
		.cursor = NULL,
		.line = 0, /* disable line:col prefix if error occured */
		.col = 1,
		.total_error = 0,
		.virtual_pc = 0,
		.symbols = { .symbol_count = 0 }

	};

	if (argc == 1) {
		compiler_error(&ctx, "No input files");
		return 1;
	}

	ctx.path = argv[1];
	pocol_compile_file(&ctx, output);
}
