/* jit.h -- Just-In-Time compilation for Pocol VM */

/* Copyright (C) 2026 Bayu Setiawan and Rasya Andrean
   This file is part of Pocol, the Pocol Virtual Machine.
   SPDX-License-Identifier: MIT
*/

#ifndef POCOL_JIT_H
#define POCOL_JIT_H

#include "vm.h"
#include <stdint.h>

/* JIT compilation mode */
typedef enum {
    JIT_MODE_DISABLED = 0,  /* Use interpreter */
    JIT_MODE_ENABLED,       /* Compile and execute native code */
    JIT_MODE_TRACE,         /* Trace execution and compile hot paths */
} JitMode;

/* Optimization level */
typedef enum {
    OPT_LEVEL_NONE = 0,     /* No optimization */
    OPT_LEVEL_BASIC,        /* Constant folding, dead code elimination */
    OPT_LEVEL_ADVANCED,     /* Peephole optimizations, register allocation */
} OptLevel;

/* JIT compiled function signature */
typedef void (*JitFunction)(PocolVM *vm);

/* JIT cache entry */
typedef struct {
    Inst_Addr start_pc;     /* Starting program counter */
    Inst_Addr end_pc;       /* Ending program counter */
    JitFunction code;       /* Compiled machine code */
    size_t code_size;       /* Size of compiled code */
    unsigned int hits;      /* Execution count for tracing */
    unsigned int compiled : 1; /* Whether this block is compiled */
} JitCacheEntry;

/* Maximum JIT cache entries */
#define JIT_CACHE_SIZE 256

/* JIT compiler context */
typedef struct {
    JitMode mode;
    OptLevel opt_level;
    JitCacheEntry cache[JIT_CACHE_SIZE];
    size_t cache_count;
    
    /* Memory for generated code */
    uint8_t *code_buffer;
    size_t buffer_size;
    size_t buffer_used;
    
    /* Statistics */
    unsigned long compile_count;
    unsigned long execute_count;
} JitContext;

/* Initialize JIT context */
void pocol_jit_init(JitContext *jit_ctx, JitMode mode, OptLevel opt_level);

/* Free JIT context */
void pocol_jit_free(JitContext *jit_ctx);

/* Compile a code block starting at pc */
Err pocol_jit_compile_block(JitContext *jit_ctx, PocolVM *vm, Inst_Addr start_pc);

/* Execute using JIT compilation */
Err pocol_jit_execute_program(JitContext *jit_ctx, PocolVM *vm, int limit);

/* Execute a single JIT-compiled block */
Err pocol_jit_execute_block(JitContext *jit_ctx, PocolVM *vm, Inst_Addr pc);

/* Find cached JIT function for given PC */
JitCacheEntry *pocol_jit_find_cache(JitContext *jit_ctx, Inst_Addr pc);

/* Simple optimizer functions */
Err pocol_optimize_bytecode(PocolVM *vm, OptLevel level);

/* Constant folding optimization */
Err pocol_opt_fold_constants(PocolVM *vm);

/* Dead code elimination */
Err pocol_opt_eliminate_dead_code(PocolVM *vm);

/* Peephole optimizations */
Err pocol_opt_peephole(PocolVM *vm);

/* Print JIT statistics */
void pocol_jit_print_stats(JitContext *jit_ctx);

#endif /* POCOL_JIT_H */