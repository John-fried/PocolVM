/* jit.c -- Just-In-Time compilation implementation for Pocol VM */

/* Copyright (C) 2026 Bayu Setiawan and Rasya Andrean
   This file is part of Pocol, the Pocol Virtual Machine.
   SPDX-License-Identifier: MIT
*/

#include "jit.h"
#include "../common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

/* Windows memory protection constants */
#ifdef _WIN32
#ifndef MEM_COMMIT
#define MEM_COMMIT     0x1000
#define MEM_RESERVE    0x2000
#define MEM_RELEASE    0x8000
#define PAGE_EXECUTE_READWRITE 0x40
#endif
#endif

/* x86-64 Register mapping for Pocol registers */
#define RAX_MAP 0
#define RCX_MAP 1
#define RDX_MAP 2
#define RBX_MAP 3
#define RSP_MAP 4
#define RBP_MAP 5
#define RSI_MAP 6
#define RDI_MAP 7

/* x86-64 instruction encoding helpers */
static inline void emit_byte(uint8_t **code_ptr, uint8_t byte) {
    **code_ptr = byte;
    (*code_ptr)++;
}

static inline void emit_word(uint8_t **code_ptr, uint16_t word) {
    memcpy(*code_ptr, &word, sizeof(word));
    *code_ptr += sizeof(word);
}

static inline void emit_dword(uint8_t **code_ptr, uint32_t dword) {
    memcpy(*code_ptr, &dword, sizeof(dword));
    *code_ptr += sizeof(dword);
}

static inline void emit_qword(uint8_t **code_ptr, uint64_t qword) {
    memcpy(*code_ptr, &qword, sizeof(qword));
    *code_ptr += sizeof(qword);
}

/* Emit MOV reg, imm64 */
static inline void emit_mov_reg_imm64(uint8_t **code_ptr, uint8_t reg, uint64_t imm) {
    emit_byte(code_ptr, 0x48);  /* REX.W prefix */
    emit_byte(code_ptr, 0xB8 + reg);  /* MOV reg, imm64 */
    emit_qword(code_ptr, imm);
}

/* Emit MOV [reg+offset], reg */
static inline void emit_mov_mem_reg(uint8_t **code_ptr, uint8_t base_reg, int32_t offset, uint8_t src_reg) {
    emit_byte(code_ptr, 0x48);  /* REX.W prefix */
    emit_byte(code_ptr, 0x89);  /* MOV [reg], reg */
    if (offset == 0) {
        emit_byte(code_ptr, 0x00 + (src_reg << 3) + base_reg);  /* ModR/M */
    } else if (offset >= -128 && offset <= 127) {
        emit_byte(code_ptr, 0x40 + (src_reg << 3) + base_reg);  /* ModR/M + disp8 */
        emit_byte(code_ptr, (uint8_t)offset);
    } else {
        emit_byte(code_ptr, 0x80 + (src_reg << 3) + base_reg);  /* ModR/M + disp32 */
        emit_dword(code_ptr, (uint32_t)offset);
    }
}

/* Emit MOV reg, [reg+offset] */
static inline void emit_mov_reg_mem(uint8_t **code_ptr, uint8_t dst_reg, uint8_t base_reg, int32_t offset) {
    emit_byte(code_ptr, 0x48);  /* REX.W prefix */
    emit_byte(code_ptr, 0x8B);  /* MOV reg, [reg] */
    if (offset == 0) {
        emit_byte(code_ptr, 0x00 + (dst_reg << 3) + base_reg);  /* ModR/M */
    } else if (offset >= -128 && offset <= 127) {
        emit_byte(code_ptr, 0x40 + (dst_reg << 3) + base_reg);  /* ModR/M + disp8 */
        emit_byte(code_ptr, (uint8_t)offset);
    } else {
        emit_byte(code_ptr, 0x80 + (dst_reg << 3) + base_reg);  /* ModR/M + disp32 */
        emit_dword(code_ptr, (uint32_t)offset);
    }
}

/* Emit ADD reg, reg */
static inline void emit_add_reg_reg(uint8_t **code_ptr, uint8_t dst_reg, uint8_t src_reg) {
    emit_byte(code_ptr, 0x48);  /* REX.W prefix */
    emit_byte(code_ptr, 0x01);  /* ADD reg, reg */
    emit_byte(code_ptr, 0xC0 + (src_reg << 3) + dst_reg);  /* ModR/M */
}

/* Emit PUSH reg */
static inline void emit_push_reg(uint8_t **code_ptr, uint8_t reg) {
    emit_byte(code_ptr, 0x50 + reg);
}

/* Emit POP reg */
static inline void emit_pop_reg(uint8_t **code_ptr, uint8_t reg) {
    emit_byte(code_ptr, 0x58 + reg);
}

/* Emit CALL rel32 */
static inline void emit_call_rel32(uint8_t **code_ptr, int32_t offset) {
    emit_byte(code_ptr, 0xE8);
    emit_dword(code_ptr, (uint32_t)offset);
}

/* Emit RET */
static inline void emit_ret(uint8_t **code_ptr) {
    emit_byte(code_ptr, 0xC3);
}

/* Map Pocol register to x86-64 register */
static inline uint8_t map_register(uint8_t pocol_reg) {
    /* Simple mapping: r0-r7 -> rax,rcx,rdx,rbx,rsp,rbp,rsi,rdi */
    static const uint8_t reg_map[] = {RAX_MAP, RCX_MAP, RDX_MAP, RBX_MAP, RSP_MAP, RBP_MAP, RSI_MAP, RDI_MAP};
    return reg_map[pocol_reg & 0x07];
}

void pocol_jit_init(JitContext *jit_ctx, JitMode mode, OptLevel opt_level) {
    memset(jit_ctx, 0, sizeof(JitContext));
    jit_ctx->mode = mode;
    jit_ctx->opt_level = opt_level;
    
    /* Allocate executable memory for JIT code */
    jit_ctx->buffer_size = 1024 * 1024;  /* 1MB initial buffer */
    
#ifdef _WIN32
    jit_ctx->code_buffer = VirtualAlloc(NULL, jit_ctx->buffer_size, 
                                       MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
    jit_ctx->code_buffer = mmap(NULL, jit_ctx->buffer_size, 
                               PROT_READ | PROT_WRITE | PROT_EXEC,
                               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
    
    if (!jit_ctx->code_buffer) {
        pocol_error("Failed to allocate JIT code buffer\n");
        exit(1);
    }
    
    jit_ctx->buffer_used = 0;
    jit_ctx->cache_count = 0;
    jit_ctx->compile_count = 0;
    jit_ctx->execute_count = 0;
}

void pocol_jit_free(JitContext *jit_ctx) {
    if (jit_ctx->code_buffer) {
#ifdef _WIN32
        VirtualFree(jit_ctx->code_buffer, 0, MEM_RELEASE);
#else
        munmap(jit_ctx->code_buffer, jit_ctx->buffer_size);
#endif
    }
    memset(jit_ctx, 0, sizeof(JitContext));
}

JitCacheEntry *pocol_jit_find_cache(JitContext *jit_ctx, Inst_Addr pc) {
    for (size_t i = 0; i < jit_ctx->cache_count; i++) {
        if (pc >= jit_ctx->cache[i].start_pc && pc <= jit_ctx->cache[i].end_pc) {
            return &jit_ctx->cache[i];
        }
    }
    return NULL;
}

static Err compile_instruction(PocolVM *vm, uint8_t **code_ptr, Inst_Addr *pc) {
    if (*pc >= POCOL_MEMORY_SIZE) {
        return ERR_ILLEGAL_INST_ACCESS;
    }
    
    uint8_t op = vm->memory[(*pc)++];
    uint8_t desc = vm->memory[(*pc)++];
    uint8_t op1 = DESC_GET_OP1(desc);
    uint8_t op2 = DESC_GET_OP2(desc);
    
    switch (op) {
        case INST_HALT:
            /* Set vm->halt = 1 */
            {
                unsigned int *halt_ptr = &vm->halt;
                emit_mov_reg_imm64(code_ptr, RAX_MAP, (uint64_t)halt_ptr);
            }
            emit_byte(code_ptr, 0xC6);  /* MOV byte ptr [rax], 1 */
            emit_byte(code_ptr, 0x00);
            emit_byte(code_ptr, 0x01);
            break;
            
        case INST_PUSH: {
            /* Load operand into RDX */
            if (op1 == OPR_REG) {
                uint8_t reg_idx = vm->memory[(*pc)++] & 0x07;
                emit_mov_reg_mem(code_ptr, RDX_MAP, RAX_MAP, 
                                ((char*)&vm->registers - (char*)vm) + (reg_idx * 8));
            } else if (op1 == OPR_IMM) {
                uint64_t imm_val;
                memcpy(&imm_val, &vm->memory[*pc], sizeof(uint64_t));
                *pc += 8;
                emit_mov_reg_imm64(code_ptr, RDX_MAP, imm_val);
            }
            
            /* Check stack overflow */
            emit_mov_reg_mem(code_ptr, RCX_MAP, RAX_MAP, offsetof(PocolVM, sp));
            /* Stack overflow check - simplified for now */
            emit_mov_reg_imm64(code_ptr, RDX_MAP, POCOL_STACK_SIZE);
            emit_cmp_rcx_rdx(code_ptr);
            /* TODO: Add jump to error handler if overflow */
            
            /* Push value to stack */
            emit_mov_reg_mem(code_ptr, RCX_MAP, RAX_MAP, ((char*)&vm->sp - (char*)vm));
            emit_mov_reg_imm64(code_ptr, RSI_MAP, (uint64_t)vm->stack);
            emit_add_reg_reg(code_ptr, RSI_MAP, RCX_MAP);  /* RSI = stack + sp */
            emit_mov_mem_reg(code_ptr, RSI_MAP, 0, RDX_MAP);  /* stack[sp] = value */
            
            /* Increment sp */
            emit_inc_rcx(code_ptr);
            emit_mov_mem_reg(code_ptr, RAX_MAP, offsetof(PocolVM, sp), RCX_MAP);
            break;
        }
        
        case INST_POP: {
            uint8_t reg_idx = vm->memory[(*pc)++] & 0x07;
            
            /* Check stack underflow */
            emit_mov_reg_mem(code_ptr, RCX_MAP, RAX_MAP, ((char*)&vm->sp - (char*)vm));
            /* TODO: Add jump to error handler if underflow */
            
            /* Pop value from stack */
            emit_dec_rcx(code_ptr);
            emit_mov_mem_reg(code_ptr, RAX_MAP, offsetof(PocolVM, sp), RCX_MAP);
            emit_mov_reg_imm64(code_ptr, RSI_MAP, (uint64_t)vm->stack);
            emit_add_reg_reg(code_ptr, RSI_MAP, RCX_MAP);  /* RSI = stack + sp */
            emit_mov_reg_mem(code_ptr, RDX_MAP, RSI_MAP, 0);  /* value = stack[sp] */
            
            /* Store in register */
            emit_mov_mem_reg(code_ptr, RAX_MAP, 
                           ((char*)&vm->registers - (char*)vm) + (reg_idx * 8), RDX_MAP);
            break;
        }
        
        case INST_ADD: {
            uint8_t dst_reg_idx = vm->memory[(*pc)++] & 0x07;
            
            /* Load destination register */
            emit_mov_reg_mem(code_ptr, RDX_MAP, RAX_MAP, 
                           ((char*)&vm->registers - (char*)vm) + (dst_reg_idx * 8));
            
            /* Load source operand */
            if (op2 == OPR_REG) {
                uint8_t src_reg_idx = vm->memory[(*pc)++] & 0x07;
                emit_mov_reg_mem(code_ptr, RCX_MAP, RAX_MAP, 
                               ((char*)&vm->registers - (char*)vm) + (src_reg_idx * 8));
            } else if (op2 == OPR_IMM) {
                uint64_t imm_val;
                memcpy(&imm_val, &vm->memory[*pc], sizeof(uint64_t));
                *pc += 8;
                emit_mov_reg_imm64(code_ptr, RCX_MAP, imm_val);
            }
            
            /* Perform addition */
            emit_add_reg_reg(code_ptr, RDX_MAP, RCX_MAP);
            
            /* Store result */
            emit_mov_mem_reg(code_ptr, RAX_MAP, 
                           ((char*)&vm->registers - (char*)vm) + (dst_reg_idx * 8), RDX_MAP);
            break;
        }
        
        case INST_JMP: {
            if (op1 == OPR_IMM) {
                uint64_t target_pc;
                memcpy(&target_pc, &vm->memory[*pc], sizeof(uint64_t));
                *pc += 8;
                
                /* Set new PC */
                emit_mov_reg_imm64(code_ptr, RDX_MAP, target_pc);
                emit_mov_mem_reg(code_ptr, RAX_MAP, ((char*)&vm->pc - (char*)vm), RDX_MAP);
                
                /* Jump to epilogue to continue execution */
                /* We'll handle this by returning and letting the executor call next block */
            }
            break;
        }
        
        case INST_PRINT: {
            /* Load operand */
            if (op1 == OPR_REG) {
                uint8_t reg_idx = vm->memory[(*pc)++] & 0x07;
                emit_mov_reg_mem(code_ptr, RDI_MAP, RAX_MAP, 
                               ((char*)&vm->registers - (char*)vm) + (reg_idx * 8));
            } else if (op1 == OPR_IMM) {
                uint64_t imm_val;
                memcpy(&imm_val, &vm->memory[*pc], sizeof(uint64_t));
                *pc += 8;
                emit_mov_reg_imm64(code_ptr, RDI_MAP, imm_val);
            }
            
            /* Call printf */
            emit_mov_reg_imm64(code_ptr, RAX_MAP, (uint64_t)printf);
            emit_call_rel32(code_ptr, (int32_t)((uint8_t*)printf - *code_ptr - 5));
            break;
        }
        
        default:
            return ERR_ILLEGAL_INST;
    }
    
    return ERR_OK;
}

/* Helper functions for x86-64 instruction emission */
static inline void emit_cmp_rcx_imm32(uint8_t **code_ptr, uint32_t imm) {
    emit_byte(code_ptr, 0x48);  /* REX.W */
    emit_byte(code_ptr, 0x81);  /* CMP reg, imm32 */
    emit_byte(code_ptr, 0xF9);  /* ModR/M: CMP RCX, imm32 */
    emit_dword(code_ptr, imm);
}

static inline void emit_cmp_rcx_rdx(uint8_t **code_ptr) {
    emit_byte(code_ptr, 0x48);  /* REX.W */
    emit_byte(code_ptr, 0x39);  /* CMP reg, reg */
    emit_byte(code_ptr, 0xD1);  /* ModR/M: CMP RCX, RDX */
}

static inline void emit_inc_rcx(uint8_t **code_ptr) {
    emit_byte(code_ptr, 0x48);  /* REX.W */
    emit_byte(code_ptr, 0xFF);  /* INC reg */
    emit_byte(code_ptr, 0xC1);  /* ModR/M: INC RCX */
}

static inline void emit_dec_rcx(uint8_t **code_ptr) {
    emit_byte(code_ptr, 0x48);  /* REX.W */
    emit_byte(code_ptr, 0xFF);  /* DEC reg */
    emit_byte(code_ptr, 0xC9);  /* ModR/M: DEC RCX */
}

Err pocol_jit_compile_block(JitContext *jit_ctx, PocolVM *vm, Inst_Addr start_pc) {
    if (jit_ctx->cache_count >= JIT_CACHE_SIZE) {
        return ERR_OK;  /* Cache full, use interpreter */
    }
    
    uint8_t *code_start = jit_ctx->code_buffer + jit_ctx->buffer_used;
    uint8_t *code_ptr = code_start;
    
    /* Save VM pointer in RAX for easy access to fields */
    emit_mov_reg_imm64(&code_ptr, RAX_MAP, (uint64_t)vm);
    
    Inst_Addr current_pc = start_pc;
    Inst_Addr end_pc = start_pc;
    
    /* Compile instructions until HALT or control flow change */
    while (current_pc < POCOL_MEMORY_SIZE) {
        uint8_t op = vm->memory[current_pc];
        if (op == INST_HALT) {
            end_pc = current_pc;
            break;
        }
        
        /* For simplicity, compile one instruction at a time for now */
        Err err = compile_instruction(vm, &code_ptr, &current_pc);
        if (err != ERR_OK) {
            return err;
        }
        
        end_pc = current_pc;
        
        /* For now, stop at control flow changes */
        if (op == INST_JMP) {
            break;
        }
    }
    
    /* Add epilogue */
    emit_ret(&code_ptr);
    
    /* Create cache entry */
    JitCacheEntry *entry = &jit_ctx->cache[jit_ctx->cache_count++];
    entry->start_pc = start_pc;
    entry->end_pc = end_pc;
    entry->code = (JitFunction)code_start;
    entry->code_size = code_ptr - code_start;
    entry->hits = 0;
    entry->compiled = 1;
    
    jit_ctx->buffer_used += entry->code_size;
    jit_ctx->compile_count++;
    
    return ERR_OK;
}

Err pocol_jit_execute_block(JitContext *jit_ctx, PocolVM *vm, Inst_Addr pc) {
    JitCacheEntry *entry = pocol_jit_find_cache(jit_ctx, pc);
    
    if (!entry) {
        /* Compile the block */
        Err err = pocol_jit_compile_block(jit_ctx, vm, pc);
        if (err != ERR_OK) {
            return err;
        }
        entry = pocol_jit_find_cache(jit_ctx, pc);
    }
    
    if (entry && entry->compiled) {
        entry->hits++;
        jit_ctx->execute_count++;
        entry->code(vm);
        return ERR_OK;
    }
    
    /* Fall back to interpreter */
    return pocol_execute_inst(vm);
}

Err pocol_jit_execute_program(JitContext *jit_ctx, PocolVM *vm, int limit) {
    while (limit != 0 && !vm->halt) {
        Err err = pocol_jit_execute_block(jit_ctx, vm, vm->pc);
        if (err != ERR_OK) {
            pocol_error("JIT execution error at addr: %u\n", vm->pc);
            return err;
        }
        if (limit > 0)
            --limit;
    }
    
    return ERR_OK;
}

void pocol_jit_print_stats(JitContext *jit_ctx) {
    printf("=== JIT Statistics ===\n");
    printf("Mode: %s\n", 
           jit_ctx->mode == JIT_MODE_DISABLED ? "Disabled" :
           jit_ctx->mode == JIT_MODE_ENABLED ? "Enabled" : "Trace");
    printf("Optimization Level: %s\n",
           jit_ctx->opt_level == OPT_LEVEL_NONE ? "None" :
           jit_ctx->opt_level == OPT_LEVEL_BASIC ? "Basic" : "Advanced");
    printf("Compiled blocks: %lu\n", jit_ctx->compile_count);
    printf("Executed blocks: %lu\n", jit_ctx->execute_count);
    printf("Cache entries: %zu/%d\n", jit_ctx->cache_count, JIT_CACHE_SIZE);
    printf("Code buffer used: %zu/%zu bytes\n", jit_ctx->buffer_used, jit_ctx->buffer_size);
    
    if (jit_ctx->cache_count > 0) {
        printf("\nCached blocks:\n");
        for (size_t i = 0; i < jit_ctx->cache_count; i++) {
            printf("  [%zu] PC %u-%u: %zu bytes, %u hits\n",
                   i, jit_ctx->cache[i].start_pc, jit_ctx->cache[i].end_pc,
                   jit_ctx->cache[i].code_size, jit_ctx->cache[i].hits);
        }
    }
}
