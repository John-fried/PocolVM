#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pocolvm.h"
#include "compiler.h"

void print_usage()
{
	printf("Usage:\n");
	printf("  Run binary:       pocol <file_binary>\n");
	printf("  COmpile source:   pocol compile <file_source.pcl>\n");
}

int main(int argc, char **argv)
{
	if (argc == 3 && strcmp(argv[1], "compile") == 0) {
		char *input_path = argv[2];
		char *output_path = "out.pob";	// .pob = Pocol Binary

		if (pocol_compile_file(input_path, output_path) == 0)
			return 0;
		else
			return 1;
	}
	else if (argc == 2) {
		PocolVM *vm;
		Err err;

		if (pocol_load_program_into_vm(argv[1], &vm) == 0) {
			err = pocol_execute_program(vm, -1);
			if (err != ERR_OK) {
				fprintf(stderr, "\nRuntime Error: %d\n", err);
			}

			putchar('\n');
			pocol_free_vm(vm);
		}
		return (int)err;
	}
	else {
		print_usage();
		return 1;
	}
}
