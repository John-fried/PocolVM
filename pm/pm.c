#include "vm.h"
#include <stdlib.h>

int main(int argc, char **argv)
{
	if (argc == 1) {
		pocol_error("no input files\n");
		return 1;
	}

	PocolVM *vm = NULL;
	Err err = ERR_OK;

	if (pocol_load_program_into_vm(argv[1], &vm) == 0) {
		err = pocol_execute_program(vm, (argc > 2) ? atoi(argv[2]) : -1); /* -1: no limit */
		pocol_free_vm(vm);
	}

	return (int)err;
}
