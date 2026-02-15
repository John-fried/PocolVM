/* vm_debugger.h -- PocolVM Debugger Interface */

/* Copyright (C) 2026 Bayu Setiawan and Rasya Andrean */

#ifndef POCOL_VM_DEBUGGER_H
#define POCOL_VM_DEBUGGER_H

#include "vm.h"
#include <stdint.h>
#include <stdbool.h>

/* Debugger Configuration */
#define DEBUG_MAX_BREAKPOINTS    64
#define DEBUG_MAX_WATCHPOINTS    32
#define DEBUG_MAX_HISTORY        256

/* Debugger Modes */
typedef enum {
    DEBUG_MODE_RUN,
    DEBUG_MODE_STEP_IN,
    DEBUG_MODE_STEP_OVER,
    DEBUG_MODE_STEP_OUT,
    DEBUG_MODE_BREAK,
    DEBUG_MODE_WATCH,
    DEBUG_MODE_FINISHED
} DebugMode;

/* Breakpoint */
typedef struct {
    Inst_Addr address;
    bool enabled;
    bool one_shot;
    int hit_count;
} BreakPoint;

/* Watchpoint */
typedef enum { WATCH_READ, WATCH_WRITE, WATCH_ACCESS } WatchType;

typedef struct {
    Inst_Addr address;
    uint64_t size;
    WatchType type;
    bool enabled;
    int hit_count;
} WatchPoint;

/* Execution State */
typedef struct {
    Inst_Addr pc;
    uint64_t registers[8];
    Stack_Addr sp;
    uint64_t stack[16];
    int stack_count;
    uint64_t instruction_count;
} ExecutionState;

/* Disassembly Info */
typedef struct {
    Inst_Addr address;
    Inst_Type type;
    const char *name;
    int operand;
} DisasmInfo;

/* Call Frame */
typedef struct CallFrame {
    Inst_Addr return_addr;
    Inst_Addr function_start;
    int frame_depth;
    struct CallFrame *next;
} CallFrame;

/* Debugger Context */
typedef struct {
    DebugMode mode;
    bool running;
    bool initialized;
    BreakPoint breakpoints[DEBUG_MAX_BREAKPOINTS];
    int breakpoint_count;
    WatchPoint watchpoints[DEBUG_MAX_WATCHPOINTS];
    int watchpoint_count;
    ExecutionState current_state;
    ExecutionState previous_state;
    CallFrame *call_stack;
    int call_stack_depth;
    ExecutionState history[DEBUG_MAX_HISTORY];
    int history_index;
    int history_count;
    int steps_remaining;
    Inst_Addr step_out_addr;
    int call_depth_target;
    bool quiet_mode;
    bool show_registers;
    bool show_stack;
    bool show_memory;
    bool show_disasm;
    int memory_display_lines;
    uint64_t total_instructions;
    PocolVM *vm;
} DebuggerContext;

/* Functions */
void debugger_init(DebuggerContext *ctx, PocolVM *vm);
void debugger_free(DebuggerContext *ctx);
void debugger_reset(DebuggerContext *ctx);

int debugger_add_breakpoint(DebuggerContext *ctx, Inst_Addr addr);
int debugger_remove_breakpoint(DebuggerContext *ctx, Inst_Addr addr);
int debugger_enable_breakpoint(DebuggerContext *ctx, Inst_Addr addr);
int debugger_disable_breakpoint(DebuggerContext *ctx, Inst_Addr addr);
void debugger_list_breakpoints(DebuggerContext *ctx);
BreakPoint* debugger_find_breakpoint(DebuggerContext *ctx, Inst_Addr addr);

int debugger_add_watchpoint(DebuggerContext *ctx, Inst_Addr addr, uint64_t size, WatchType type);
int debugger_remove_watchpoint(DebuggerContext *ctx, Inst_Addr addr);
void debugger_list_watchpoints(DebuggerContext *ctx);

void debugger_run(DebuggerContext *ctx);
void debugger_continue(DebuggerContext *ctx);
void debugger_step_into(DebuggerContext *ctx, int count);
void debugger_step_over(DebuggerContext *ctx, int count);
void debugger_step_out(DebuggerContext *ctx);
void debugger_stop(DebuggerContext *ctx);

void debugger_save_state(DebuggerContext *ctx);
void debugger_restore_state(DebuggerContext *ctx);
void debugger_clear_history(DebuggerContext *ctx);

void debugger_show_registers(DebuggerContext *ctx);
void debugger_show_stack(DebuggerContext *ctx, int count);
void debugger_show_memory(DebuggerContext *ctx, Inst_Addr addr, int count);
void debugger_show_disasm(DebuggerContext *ctx, Inst_Addr addr, int count);
void debugger_show_callstack(DebuggerContext *ctx);
void debugger_show_state(DebuggerContext *ctx);

void debugger_disasm_instruction(DebuggerContext *ctx, Inst_Addr addr, DisasmInfo *info);
void debugger_disasm_range(DebuggerContext *ctx, Inst_Addr start, Inst_Addr end);

const char* debugger_get_inst_name(Inst_Type type);
void debugger_prompt(DebuggerContext *ctx);
bool debugger_should_stop(DebuggerContext *ctx);
void debugger_check_watchpoints(DebuggerContext *ctx);

void debugger_visualize(DebuggerContext *ctx);
void debugger_visualize_memory(DebuggerContext *ctx, Inst_Addr start, int rows);
void debugger_visualize_stack(DebuggerContext *ctx);
void debugger_visualize_registers(DebuggerContext *ctx);

#endif