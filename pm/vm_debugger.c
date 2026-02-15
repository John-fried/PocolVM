/* vm_debugger.c -- PocolVM Debugger Implementation */

/* Copyright (C) 2026 Bayu Setiawan and Rasya Andrean */

#include "vm_debugger.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Instruction names */
static const char* inst_names[] = {"HALT", "PUSH", "POP", "ADD", "JMP", "PRINT", "SYS"};
static const char* inst_mnemonics[] = {"halt", "push", "pop", "add", "jmp", "print", "sys"};

/* Initialization */
void debugger_init(DebuggerContext *ctx, PocolVM *vm) {
    memset(ctx, 0, sizeof(DebuggerContext));
    ctx->vm = vm;
    ctx->running = true;
    ctx->initialized = true;
    ctx->mode = DEBUG_MODE_RUN;
    ctx->show_registers = true;
    ctx->show_stack = true;
    ctx->show_disasm = true;
    ctx->memory_display_lines = 8;
    ctx->quiet_mode = false;
    ctx->call_stack = NULL;
    ctx->history_index = 0;
    ctx->history_count = 0;
}

void debugger_free(DebuggerContext *ctx) {
    if (!ctx || !ctx->initialized) return;
    CallFrame *frame = ctx->call_stack;
    while (frame) {
        CallFrame *next = frame->next;
        free(frame);
        frame = next;
    }
    ctx->initialized = false;
}

void debugger_reset(DebuggerContext *ctx) {
    if (!ctx || !ctx->initialized) return;
    ctx->mode = DEBUG_MODE_RUN;
    ctx->running = true;
    ctx->steps_remaining = 0;
    ctx->breakpoint_count = 0;
    ctx->watchpoint_count = 0;
    ctx->history_index = 0;
    ctx->history_count = 0;
}

/* Breakpoints */
int debugger_add_breakpoint(DebuggerContext *ctx, Inst_Addr addr) {
    if (!ctx || !ctx->initialized) return -1;
    if (ctx->breakpoint_count >= DEBUG_MAX_BREAKPOINTS) return -1;
    for (int i = 0; i < ctx->breakpoint_count; i++) {
        if (ctx->breakpoints[i].address == addr) {
            ctx->breakpoints[i].enabled = true;
            return i;
        }
    }
    BreakPoint *bp = &ctx->breakpoints[ctx->breakpoint_count++];
    bp->address = addr;
    bp->enabled = true;
    bp->one_shot = false;
    bp->hit_count = 0;
    return ctx->breakpoint_count - 1;
}

int debugger_remove_breakpoint(DebuggerContext *ctx, Inst_Addr addr) {
    if (!ctx || !ctx->initialized) return -1;
    for (int i = 0; i < ctx->breakpoint_count; i++) {
        if (ctx->breakpoints[i].address == addr) {
            for (int j = i; j < ctx->breakpoint_count - 1; j++) {
                ctx->breakpoints[j] = ctx->breakpoints[j + 1];
            }
            ctx->breakpoint_count--;
            return 0;
        }
    }
    return -1;
}

int debugger_enable_breakpoint(DebuggerContext *ctx, Inst_Addr addr) {
    BreakPoint *bp = debugger_find_breakpoint(ctx, addr);
    if (bp) { bp->enabled = true; return 0; }
    return -1;
}

int debugger_disable_breakpoint(DebuggerContext *ctx, Inst_Addr addr) {
    BreakPoint *bp = debugger_find_breakpoint(ctx, addr);
    if (bp) { bp->enabled = false; return 0; }
    return -1;
}

BreakPoint* debugger_find_breakpoint(DebuggerContext *ctx, Inst_Addr addr) {
    if (!ctx || !ctx->initialized) return NULL;
    for (int i = 0; i < ctx->breakpoint_count; i++) {
        if (ctx->breakpoints[i].address == addr) return &ctx->breakpoints[i];
    }
    return NULL;
}

void debugger_list_breakpoints(DebuggerContext *ctx) {
    if (!ctx || !ctx->initialized) return;
    printf("\n=== Breakpoints ===\n");
    if (ctx->breakpoint_count == 0) { printf("No breakpoints set.\n"); return; }
    for (int i = 0; i < ctx->breakpoint_count; i++) {
        BreakPoint *bp = &ctx->breakpoints[i];
        printf("[%d] Address: 0x%04X %s (hit: %d)\n", i, bp->address, bp->enabled ? "enabled" : "disabled", bp->hit_count);
    }
}

/* Watchpoints */
int debugger_add_watchpoint(DebuggerContext *ctx, Inst_Addr addr, uint64_t size, WatchType type) {
    if (!ctx || !ctx->initialized) return -1;
    if (ctx->watchpoint_count >= DEBUG_MAX_WATCHPOINTS) return -1;
    WatchPoint *wp = &ctx->watchpoints[ctx->watchpoint_count++];
    wp->address = addr; wp->size = size; wp->type = type;
    wp->enabled = true; wp->hit_count = 0;
    return ctx->watchpoint_count - 1;
}

int debugger_remove_watchpoint(DebuggerContext *ctx, Inst_Addr addr) {
    if (!ctx || !ctx->initialized) return -1;
    for (int i = 0; i < ctx->watchpoint_count; i++) {
        if (ctx->watchpoints[i].address == addr) {
            for (int j = i; j < ctx->watchpoint_count - 1; j++) ctx->watchpoints[j] = ctx->watchpoints[j + 1];
            ctx->watchpoint_count--;
            return 0;
        }
    }
    return -1;
}

void debugger_list_watchpoints(DebuggerContext *ctx) {
    if (!ctx || !ctx->initialized) return;
    printf("\n=== Watchpoints ===\n");
    if (ctx->watchpoint_count == 0) { printf("No watchpoints set.\n"); return; }
    static const char *watch_types[] = {"READ", "WRITE", "ACCESS"};
    for (int i = 0; i < ctx->watchpoint_count; i++) {
        WatchPoint *wp = &ctx->watchpoints[i];
        printf("[%d] Address: 0x%04X Size: %lu Type: %s %s (hit: %d)\n", i, wp->address, (unsigned long)wp->size, watch_types[wp->type], wp->enabled ? "enabled" : "disabled", wp->hit_count);
    }
}

/* Execution Control */
void debugger_run(DebuggerContext *ctx) {
    if (!ctx || !ctx->initialized) return;
    ctx->mode = DEBUG_MODE_RUN;
    ctx->running = true;
}

void debugger_continue(DebuggerContext *ctx) {
    if (!ctx || !ctx->initialized) return;
    ctx->mode = DEBUG_MODE_RUN;
    ctx->running = true;
    ctx->steps_remaining = 0;
}

void debugger_step_into(DebuggerContext *ctx, int count) {
    if (!ctx || !ctx->initialized) return;
    ctx->mode = DEBUG_MODE_STEP_IN;
    ctx->running = true;
    ctx->steps_remaining = count;
}

void debugger_step_over(DebuggerContext *ctx, int count) {
    if (!ctx || !ctx->initialized) return;
    ctx->mode = DEBUG_MODE_STEP_OVER;
    ctx->running = true;
    ctx->steps_remaining = count;
    ctx->call_depth_target = ctx->call_stack_depth;
}

void debugger_step_out(DebuggerContext *ctx) {
    if (!ctx || !ctx->initialized) return;
    ctx->mode = DEBUG_MODE_STEP_OUT;
    ctx->running = true;
}

void debugger_stop(DebuggerContext *ctx) {
    if (!ctx || !ctx->initialized) return;
    ctx->running = false;
    ctx->mode = DEBUG_MODE_BREAK;
}

/* State Management */
void debugger_save_state(DebuggerContext *ctx) {
    if (!ctx || !ctx->initialized || !ctx->vm) return;
    PocolVM *vm = ctx->vm;
    ctx->previous_state = ctx->current_state;
    ctx->current_state.pc = vm->pc;
    memcpy(ctx->current_state.registers, vm->registers, sizeof(vm->registers));
    ctx->current_state.sp = vm->sp;
    ctx->current_state.stack_count = (vm->sp < 16) ? vm->sp : 16;
    for (int i = 0; i < ctx->current_state.stack_count; i++) {
        ctx->current_state.stack[i] = vm->stack[i];
    }
    int idx = ctx->history_index % DEBUG_MAX_HISTORY;
    ctx->history[idx] = ctx->current_state;
    ctx->history_index++;
    if (ctx->history_count < DEBUG_MAX_HISTORY) ctx->history_count++;
}

void debugger_restore_state(DebuggerContext *ctx) {
    if (!ctx || !ctx->initialized || !ctx->vm) return;
    if (ctx->history_count == 0) return;
    PocolVM *vm = ctx->vm;
    ctx->current_state = ctx->previous_state;
    vm->pc = ctx->current_state.pc;
    memcpy(vm->registers, ctx->current_state.registers, sizeof(vm->registers));
    vm->sp = ctx->current_state.sp;
}

void debugger_clear_history(DebuggerContext *ctx) {
    if (!ctx || !ctx->initialized) return;
    ctx->history_index = 0;
    ctx->history_count = 0;
}

/* Inspection Functions */
void debugger_show_registers(DebuggerContext *ctx) {
    if (!ctx || !ctx->initialized || !ctx->vm) return;
    printf("\n=== Registers ===\n");
    uint64_t *regs = ctx->vm->registers;
    for (int i = 0; i < 8; i++) {
        printf("r%d = %llu (0x%016llX)\n", i, (unsigned long long)regs[i], (unsigned long long)regs[i]);
    }
    printf("pc  = 0x%04X\n", ctx->vm->pc);
    printf("sp  = %d\n", ctx->vm->sp);
}

void debugger_show_stack(DebuggerContext *ctx, int count) {
    if (!ctx || !ctx->initialized || !ctx->vm) return;
    if (count <= 0) count = 16;
    if (count > POCOL_STACK_SIZE) count = POCOL_STACK_SIZE;
    printf("\n=== Stack (top %d) ===\n", count);
    printf("sp = %d\n", ctx->vm->sp);
    for (int i = count - 1; i >= 0; i--) {
        if (i < ctx->vm->sp) {
            printf("[%d] = %llu (0x%016llX)\n", i, (unsigned long long)ctx->vm->stack[i], (unsigned long long)ctx->vm->stack[i]);
        } else {
            printf("[%d] = <empty>\n", i);
        }
    }
}

void debugger_show_memory(DebuggerContext *ctx, Inst_Addr addr, int count) {
    if (!ctx || !ctx->initialized || !ctx->vm) return;
    if (count <= 0) count = 16;
    printf("\n=== Memory at 0x%04X ===\n", addr);
    for (int row = 0; row < (count + 15) / 16; row++) {
        Inst_Addr base = addr + (row * 16);
        printf("%04X: ", base);
        for (int i = 0; i < 16; i++) {
            if (base + i < POCOL_MEMORY_SIZE) {
                printf("%02X ", ctx->vm->memory[base + i]);
            } else {
                printf("   ");
            }
        }
        printf(" |");
        for (int i = 0; i < 16; i++) {
            if (base + i < POCOL_MEMORY_SIZE) {
                uint8_t byte = ctx->vm->memory[base + i];
                printf("%c", (byte >= 32 && byte < 127) ? byte : '.');
            }
        }
        printf("|\n");
    }
}

void debugger_disasm_instruction(DebuggerContext *ctx, Inst_Addr addr, DisasmInfo *info) {
    if (!ctx || !ctx->initialized || !ctx->vm || !info) return;
    if (addr >= POCOL_MEMORY_SIZE) return;
    info->address = addr;
    uint8_t inst = ctx->vm->memory[addr];
    if (inst >= COUNT_INST) {
        info->type = COUNT_INST;
        info->name = "UNKNOWN";
        info->operand = 0;
        return;
    }
    info->type = (Inst_Type)inst;
    info->name = inst_names[inst];
    info->operand = 0;
    if (addr + 1 < POCOL_MEMORY_SIZE) info->operand = ctx->vm->memory[addr + 1];
}

void debugger_show_disasm(DebuggerContext *ctx, Inst_Addr addr, int count) {
    if (!ctx || !ctx->initialized) return;
    if (count <= 0) count = 8;
    printf("\n=== Disassembly ===\n");
    for (int i = 0; i < count; i++) {
        DisasmInfo info;
        debugger_disasm_instruction(ctx, addr + i * 2, &info);
        printf("%04X: %-6s ", info.address, inst_mnemonics[info.type]);
        if (info.operand != 0 || info.type == INST_PUSH || info.type == INST_JMP) {
            printf("%d", info.operand);
        }
        if (info.address == ctx->vm->pc) printf(" <--");
        printf("\n");
    }
}

void debugger_show_callstack(DebuggerContext *ctx) {
    if (!ctx || !ctx->initialized) return;
    printf("\n=== Call Stack ===\n");
    CallFrame *frame = ctx->call_stack;
    int depth = 0;
    while (frame) {
        printf("[%d] Return: 0x%04X Function: 0x%04X\n", depth++, frame->return_addr, frame->function_start);
        frame = frame->next;
    }
    if (depth == 0) printf("(empty)\n");
}

void debugger_show_state(DebuggerContext *ctx) {
    if (!ctx || !ctx->initialized) return;
    printf("\n----------------------------------------\n");
    printf("=== Debugger State ===\n");
    printf("Mode: %s\n", 
           ctx->mode == DEBUG_MODE_RUN ? "RUN" :
           ctx->mode == DEBUG_MODE_STEP_IN ? "STEP_IN" :
           ctx->mode == DEBUG_MODE_STEP_OVER ? "STEP_OVER" :
           ctx->mode == DEBUG_MODE_STEP_OUT ? "STEP_OUT" :
           ctx->mode == DEBUG_MODE_BREAK ? "BREAK" : "UNKNOWN");
    printf("PC: 0x%04X\n", ctx->vm->pc);
    printf("----------------------------------------\n");
    if (ctx->show_registers) debugger_show_registers(ctx);
    if (ctx->show_stack) debugger_show_stack(ctx, 8);
    if (ctx->show_disasm) debugger_show_disasm(ctx, ctx->vm->pc, 5);
}

/* Control Flow */
bool debugger_should_stop(DebuggerContext *ctx) {
    if (!ctx || !ctx->initialized || !ctx->running) return true;
    if (!ctx->vm || ctx->vm->halt) return true;
    for (int i = 0; i < ctx->breakpoint_count; i++) {
        BreakPoint *bp = &ctx->breakpoints[i];
        if (bp->enabled && bp->address == ctx->vm->pc) {
            bp->hit_count++;
            ctx->mode = DEBUG_MODE_BREAK;
            printf("\n*** Breakpoint %d hit at 0x%04X ***\n", i, bp->address);
            return true;
        }
    }
    if (ctx->mode == DEBUG_MODE_STEP_IN) {
        ctx->steps_remaining--;
        if (ctx->steps_remaining <= 0) { ctx->mode = DEBUG_MODE_BREAK; return true; }
    }
    if (ctx->mode == DEBUG_MODE_STEP_OVER) {
        if (ctx->call_stack_depth <= ctx->call_depth_target) {
            ctx->steps_remaining--;
            if (ctx->steps_remaining <= 0) { ctx->mode = DEBUG_MODE_BREAK; return true; }
        }
    }
    if (ctx->mode == DEBUG_MODE_STEP_OUT) {
        if (ctx->call_stack_depth < ctx->call_depth_target) { ctx->mode = DEBUG_MODE_BREAK; return true; }
    }
    return false;
}

void debugger_check_watchpoints(DebuggerContext *ctx) {
    if (!ctx || !ctx->initialized || !ctx->vm) return;
    /* Watchpoint checking would go here */
}

/* Visualizer */
void debugger_visualize(DebuggerContext *ctx) {
    if (!ctx || !ctx->initialized) return;
    printf("\033[2J\033[H");
    debugger_visualize_registers(ctx);
    debugger_visualize_stack(ctx);
    debugger_visualize_memory(ctx, 0, 8);
}

void debugger_visualize_registers(DebuggerContext *ctx) {
    if (!ctx || !ctx->initialized || !ctx->vm) return;
    printf("\n=== REGISTERS ===\n");
    for (int i = 0; i < 8; i++) {
        printf("r%d: %llu\n", i, (unsigned long long)ctx->vm->registers[i]);
    }
    printf("PC: 0x%04X  SP: %d\n", ctx->vm->pc, ctx->vm->sp);
}

void debugger_visualize_stack(DebuggerContext *ctx) {
    if (!ctx || !ctx->initialized || !ctx->vm) return;
    printf("\n=== STACK ===\n");
    int show = (ctx->vm->sp < 8) ? ctx->vm->sp : 8;
    for (int i = show - 1; i >= 0; i--) {
        printf("[%d]: %llu\n", i, (unsigned long long)ctx->vm->stack[i]);
    }
}

void debugger_visualize_memory(DebuggerContext *ctx, Inst_Addr start, int rows) {
    if (!ctx || !ctx->initialized || !ctx->vm) return;
    if (rows <= 0) rows = 8;
    printf("\n=== MEMORY ===\n");
    for (int r = 0; r < rows; r++) {
        printf("%04X: ", start + r * 16);
        for (int i = 0; i < 16; i++) {
            printf("%02X ", ctx->vm->memory[start + r * 16 + i]);
        }
        printf("\n");
    }
}

/* Utility */
const char* debugger_get_inst_name(Inst_Type type) {
    if (type >= COUNT_INST) return "UNKNOWN";
    return inst_names[type];
}

void debugger_prompt(DebuggerContext *ctx) {
    if (!ctx) return;
    printf("\n(pocol-debug) ");
    fflush(stdout);
}

/* Stub for disasm range */
void debugger_disasm_range(DebuggerContext *ctx, Inst_Addr start, Inst_Addr end) {
    if (!ctx || !ctx->initialized) return;
    int count = (end - start) / 2 + 1;
    debugger_show_disasm(ctx, start, count);
}