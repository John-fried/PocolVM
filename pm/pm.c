#include "vm.h"
#include "vm_debugger.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Debugger command */
static void debugger_command(DebuggerContext *ctx, const char *cmd) {
    if (!ctx || !cmd) return;
    
    if (strcmp(cmd, "r") == 0 || strcmp(cmd, "run") == 0) {
        debugger_continue(ctx);
    } else if (strcmp(cmd, "c") == 0 || strcmp(cmd, "continue") == 0) {
        debugger_continue(ctx);
    } else if (strcmp(cmd, "s") == 0 || strcmp(cmd, "step") == 0) {
        debugger_step_into(ctx, 1);
    } else if (strcmp(cmd, "n") == 0 || strcmp(cmd, "next") == 0) {
        debugger_step_over(ctx, 1);
    } else if (strcmp(cmd, "p") == 0 || strcmp(cmd, "print") == 0) {
        debugger_show_registers(ctx);
    } else if (strcmp(cmd, "bt") == 0 || strcmp(cmd, "backtrace") == 0) {
        debugger_show_callstack(ctx);
    } else if (strncmp(cmd, "x/", 2) == 0) {
        /* Examine memory: x/16 0x1000 */
        Inst_Addr addr = 0;
        int count = 16;
        sscanf(cmd + 2, "%d 0x%X", &count, &addr);
        debugger_show_memory(ctx, addr, count);
    } else if (strncmp(cmd, "break ", 6) == 0) {
        Inst_Addr addr = 0;
        sscanf(cmd + 6, "%X", &addr);
        debugger_add_breakpoint(ctx, addr);
        printf("Breakpoint added at 0x%04X\n", addr);
    } else if (strcmp(cmd, "info breakpoints") == 0) {
        debugger_list_breakpoints(ctx);
    } else if (strcmp(cmd, "info registers") == 0) {
        debugger_show_registers(ctx);
    } else if (strcmp(cmd, "info stack") == 0) {
        debugger_show_stack(ctx, 16);
    } else if (strcmp(cmd, "q") == 0 || strcmp(cmd, "quit") == 0) {
        debugger_stop(ctx);
    } else if (strcmp(cmd, "h") == 0 || strcmp(cmd, "help") == 0) {
        printf("\n=== Debugger Commands ===\n");
        printf("r, run        - Run program\n");
        printf("c, continue  - Continue execution\n");
        printf("s, step      - Step one instruction\n");
        printf("n, next      - Step over instruction\n");
        printf("p, print     - Show registers\n");
        printf("bt           - Show call stack\n");
        printf("x/N ADDR     - Examine memory\n");
        printf("break ADDR   - Set breakpoint\n");
        printf("info breakpoints - List breakpoints\n");
        printf("info registers   - Show registers\n");
        printf("info stack      - Show stack\n");
        printf("q, quit       - Quit debugger\n");
        printf("h, help       - Show this help\n");
    } else {
        printf("Unknown command: %s (type 'help' for list)\n", cmd);
    }
}

/* Interactive debugger loop */
static void debugger_loop(DebuggerContext *ctx) {
    char cmd[256];
    
    while (ctx->running && !ctx->vm->halt) {
        debugger_show_state(ctx);
        debugger_prompt(ctx);
        
        if (fgets(cmd, sizeof(cmd), stdin)) {
            /* Remove newline */
            size_t len = strlen(cmd);
            if (len > 0 && cmd[len-1] == '\n') {
                cmd[len-1] = '\0';
            }
            
            if (strlen(cmd) > 0) {
                debugger_command(ctx, cmd);
            }
        }
        
        /* Execute while not stopped */
        if (ctx->mode != DEBUG_MODE_BREAK) {
            debugger_continue(ctx);
        }
    }
    
    printf("\nProgram finished (halted)\n");
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		pocol_error("usage: %s <program.pob> [options]\n", argv[0]);
		pocol_error("  --jit       : Enable JIT compilation\n");
		pocol_error("  --stats     : Show JIT statistics\n");
		pocol_error("  --debug     : Enable debugger\n");
		pocol_error("  --break ADDR: Set initial breakpoint\n");
		return 1;
	}
	
	int jit_enabled = 0;
	int show_stats = 0;
	int debug_enabled = 0;
	const char *program_path = NULL;
	int limit = -1;
	Inst_Addr initial_break = 0xFFFFFFFF;
	
	/* Parse arguments */
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--jit") == 0) {
			jit_enabled = 1;
		} else if (strcmp(argv[i], "--stats") == 0) {
			show_stats = 1;
		} else if (strcmp(argv[i], "--debug") == 0) {
			debug_enabled = 1;
		} else if (strncmp(argv[i], "--break=", 8) == 0) {
			sscanf(argv[i] + 8, "%X", &initial_break);
		} else if (argv[i][0] == '-') {
			pocol_error("unknown option: %s\n", argv[i]);
			return 1;
		} else if (!program_path) {
			program_path = argv[i];
		} else {
			limit = atoi(argv[i]);
		}
	}
	
	if (!program_path) {
		pocol_error("no input files\n");
		return 1;
	}
	
	PocolVM *vm = NULL;
	Err err = ERR_OK;
	
	if (pocol_load_program_into_vm(program_path, &vm) == 0) {
		if (debug_enabled) {
			/* Initialize debugger */
			DebuggerContext debugger;
			debugger_init(&debugger, vm);
			
			/* Set initial breakpoint if specified */
			if (initial_break != 0xFFFFFFFF) {
				debugger_add_breakpoint(&debugger, initial_break);
				printf("Initial breakpoint at 0x%04X\n", initial_break);
			}
			
			/* Enter debugger loop */
			debugger_loop(&debugger);
			
			debugger_free(&debugger);
		} else {
			/* Normal execution */
			err = pocol_execute_program_jit(vm, limit, jit_enabled);
			
			if (show_stats && vm->jit_context) {
				pocol_jit_print_stats((JitContext*)vm->jit_context);
			}
		}
		
		pocol_free_vm(vm);
	}
	
	return (int)err;
}
