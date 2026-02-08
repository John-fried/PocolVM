#include <stdio.h>
#include <stdint.h>
#include "pocolvm.h"

int main() {
    /* Opcode
       0: do_halt, 1: do_push, 2: do_pop, 3: do_add, 4: do_print
    */
    uint8_t program[] = {
        1, 10,      /* PUSH 10 */
        1, 20,      /* PUSH 20 */
        2, 0,       /* POP ke R0 (R0 = 20) */
        2, 1,       /* POP ke R1 (R1 = 10) */
        3, 0, 1,    /* ADD R0, R1 (R0 = 20 + 10 = 30) */
        4, 0,       /* PRINT R0 */
        0, 0        /* HALT with exit-code 0 (takes from R0) */
    };

    PocolVM *vm = pocol_make_vm(program, sizeof(program));
    if (!vm) return 1;

    printf("expected: 30\n");
    uint8_t ret = pocol_run_vm(vm);
    pocol_free_vm(vm);

    return ret;
}
