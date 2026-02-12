#include "compiler.h"

int main(int argc, char **argv)
{
	char *output = (argc > 2) ? argv[2] : "out.pob";

	if (argc == 1) {
		compiler_error("No input files");
		return 1;
	}

	pocol_compile_file(argv[1], output);
}
