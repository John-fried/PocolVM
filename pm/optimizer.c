/* optimizer.c -- Simple bytecode optimizer for Pocol VM */

/* Copyright (C) 2026 Bayu Setiawan and Rasya Andrean
   This file is part of Pocol, the Pocol Virtual Machine.
   SPDX-License-Identifier: MIT
*/

#include "jit.h"
#include "../common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Instruction analysis structure */
typedef struct {
    Inst_Type type;
    uint8_t desc;
    uint8_t operands[2][9];  /* Up to 9 bytes per operand (1 reg + 8 imm) */
    size_t operand_sizes[2];
    Inst_Addr pc;
} AnalyzedInst;

/* Read an instruction and its operands */
static Err read_instruction(PocolVM *vm, Inst_Addr pc, AnalyzedInst *inst) {
    if (pc >= POCOL_MEMORY_SIZE) {
        return ERR_ILLEGAL_INST_ACCESS;
    }
    
    inst->pc = pc;
    inst->type = (Inst_Type)vm->memory[pc++];
    
    if (pc >= POCOL_MEMORY_SIZE) {
        return ERR_ILLEGAL_INST_ACCESS;
    }
    
    inst->desc = vm->memory[pc++];
    uint8_t op1 = DESC_GET_OP1(inst->desc);
    uint8_t op2 = DESC_GET_OP2(inst->desc);
    
    /* Read first operand */
    if (op1 != OPR_NONE) {
        if (pc >= POCOL_MEMORY_SIZE) {
            return ERR_ILLEGAL_INST_ACCESS;
        }
        
        if (op1 == OPR_REG) {
            inst->operands[0][0] = vm->memory[pc++];
            inst->operand_sizes[0] = 1;
        } else if (op1 == OPR_IMM) {
            if (pc + 8 > POCOL_MEMORY_SIZE) {
                return ERR_ILLEGAL_INST_ACCESS;
            }
            memcpy(inst->operands[0], &vm->memory[pc], 8);
            pc += 8;
            inst->operand_sizes[0] = 8;
        }
    } else {
        inst->operand_sizes[0] = 0;
    }
    
    /* Read second operand */
    if (op2 != OPR_NONE) {
        if (pc >= POCOL_MEMORY_SIZE) {
            return ERR_ILLEGAL_INST_ACCESS;
        }
        
        if (op2 == OPR_REG) {
            inst->operands[1][0] = vm->memory[pc++];
            inst->operand_sizes[1] = 1;
        } else if (op2 == OPR_IMM) {
            if (pc + 8 > POCOL_MEMORY_SIZE) {
                return ERR_ILLEGAL_INST_ACCESS;
            }
            memcpy(inst->operands[1], &vm->memory[pc], 8);
            pc += 8;
            inst->operand_sizes[1] = 8;
        }
    } else {
        inst->operand_sizes[1] = 0;
    }
    
    return ERR_OK;
}

/* Write an instruction back to memory */
static Err write_instruction(PocolVM *vm, Inst_Addr *pc, const AnalyzedInst *inst) {
    if (*pc + 2 > POCOL_MEMORY_SIZE) {
        return ERR_ILLEGAL_INST_ACCESS;
    }
    
    vm->memory[(*pc)++] = (uint8_t)inst->type;
    vm->memory[(*pc)++] = inst->desc;
    
    /* Write operands */
    for (int i = 0; i < 2; i++) {
        if (inst->operand_sizes[i] > 0) {
            if (*pc + inst->operand_sizes[i] > POCOL_MEMORY_SIZE) {
                return ERR_ILLEGAL_INST_ACCESS;
            }
            memcpy(&vm->memory[*pc], inst->operands[i], inst->operand_sizes[i]);
            *pc += inst->operand_sizes[i];
        }
    }
    
    return ERR_OK;
}

/* Constant folding optimization */
Err pocol_opt_fold_constants(PocolVM *vm) {
    Inst_Addr pc = POCOL_MAGIC_SIZE;  /* Skip magic header */
    Inst_Addr write_pc = pc;
    
    while (pc < POCOL_MEMORY_SIZE && vm->memory[pc] != INST_HALT) {
        AnalyzedInst inst;
        Err err = read_instruction(vm, pc, &inst);
        if (err != ERR_OK) {
            return err;
        }
        
        /* Look for ADD with two immediate operands */
        if (inst.type == INST_ADD && 
            DESC_GET_OP1(inst.desc) == OPR_REG &&
            DESC_GET_OP2(inst.desc) == OPR_IMM) {
            
            uint8_t reg_idx = inst.operands[0][0] & 0x07;
            uint64_t imm_val;
            memcpy(&imm_val, inst.operands[1], 8);
            
            /* Look for previous LOAD/STORE patterns that might allow folding */
            /* For now, we'll just write it back unchanged */
            err = write_instruction(vm, &write_pc, &inst);
            if (err != ERR_OK) {
                return err;
            }
        } else {
            /* Write instruction unchanged */
            err = write_instruction(vm, &write_pc, &inst);
            if (err != ERR_OK) {
                return err;
            }
        }
        
        /* Advance read pointer */
        pc = inst.pc;
        /* Skip the instruction we just processed */
        pc = write_pc;
    }
    
    /* Copy HALT instruction */
    if (pc < POCOL_MEMORY_SIZE) {
        vm->memory[write_pc++] = vm->memory[pc];
    }
    
    return ERR_OK;
}

/* Dead code elimination */
Err pocol_opt_eliminate_dead_code(PocolVM *vm) {
    /* Simple dead code elimination: remove instructions that don't affect observable state */
    /* For this simple VM, we'll focus on removing redundant operations */
    
    Inst_Addr pc = POCOL_MAGIC_SIZE;
    Inst_Addr write_pc = pc;
    
    uint64_t last_values[8] = {0};  /* Track last known values in registers */
    unsigned int reg_modified[8] = {0};  /* Track if register was modified */
    
    while (pc < POCOL_MEMORY_SIZE && vm->memory[pc] != INST_HALT) {
        AnalyzedInst inst;
        Err err = read_instruction(vm, pc, &inst);
        if (err != ERR_OK) {
            return err;
        }
        
        int keep_instruction = 1;
        
        switch (inst.type) {
            case INST_ADD: {
                uint8_t dst_reg = inst.operands[0][0] & 0x07;
                
                /* If adding zero, eliminate the instruction */
                if (DESC_GET_OP2(inst.desc) == OPR_IMM) {
                    uint64_t imm_val;
                    memcpy(&imm_val, inst.operands[1], 8);
                    if (imm_val == 0) {
                        keep_instruction = 0;
                    }
                }
                
                /* Mark register as modified */
                reg_modified[dst_reg] = 1;
                break;
            }
            
            case INST_PUSH: {
                /* If pushing a register that hasn't been modified, we might optimize */
                /* But for now, keep all PUSH instructions */
                break;
            }
            
            case INST_POP: {
                uint8_t reg_idx = inst.operands[0][0] & 0x07;
                /* Clear the modified flag since we're loading from stack */
                reg_modified[reg_idx] = 0;
                break;
            }
            
            case INST_PRINT:
            case INST_JMP:
            case INST_HALT:
                /* Keep these as they affect observable behavior */
                break;
                
            default:
                break;
        }
        
        if (keep_instruction) {
            err = write_instruction(vm, &write_pc, &inst);
            if (err != ERR_OK) {
                return err;
            }
        }
        
        /* Advance pointers */
        pc = inst.pc;
    }
    
    /* Copy HALT instruction */
    if (pc < POCOL_MEMORY_SIZE) {
        vm->memory[write_pc++] = vm->memory[pc];
    }
    
    return ERR_OK;
}

/* Peephole optimization */
Err pocol_opt_peephole(PocolVM *vm) {
    /* Simple peephole optimizations:
     * - PUSH x; POP y -> MOVE y, x (if x and y are registers)
     * - ADD r, 0 -> eliminate
     * - Consecutive PUSH/POP pairs
     */
    
    Inst_Addr pc = POCOL_MAGIC_SIZE;
    Inst_Addr write_pc = pc;
    
    while (pc < POCOL_MEMORY_SIZE && vm->memory[pc] != INST_HALT) {
        AnalyzedInst inst1, inst2;
        
        /* Read first instruction */
        Err err = read_instruction(vm, pc, &inst1);
        if (err != ERR_OK) {
            return err;
        }
        
        /* Try to read second instruction */
        Inst_Addr next_pc = inst1.pc;
        err = read_instruction(vm, next_pc, &inst2);
        if (err != ERR_OK) {
            /* Just write the first instruction */
            err = write_instruction(vm, &write_pc, &inst1);
            if (err != ERR_OK) {
                return err;
            }
            pc = next_pc;
            continue;
        }
        
        int optimized = 0;
        
        /* PUSH x; POP y -> direct move if both are registers */
        if (inst1.type == INST_PUSH && inst2.type == INST_POP &&
            DESC_GET_OP1(inst1.desc) == OPR_REG &&
            DESC_GET_OP1(inst2.desc) == OPR_REG) {
            
            uint8_t src_reg = inst1.operands[0][0] & 0x07;
            uint8_t dst_reg = inst2.operands[0][0] & 0x07;
            
            if (src_reg != dst_reg) {
                /* Create MOV instruction (we'd need to add this to the ISA) */
                /* For now, keep the original instructions */
                err = write_instruction(vm, &write_pc, &inst1);
                if (err != ERR_OK) return err;
                err = write_instruction(vm, &write_pc, &inst2);
                if (err != ERR_OK) return err;
            } else {
                /* Same register, eliminate both */
                optimized = 1;
            }
        }
        /* ADD r, 0 -> eliminate */
        else if (inst1.type == INST_ADD && DESC_GET_OP2(inst1.desc) == OPR_IMM) {
            uint64_t imm_val;
            memcpy(&imm_val, inst1.operands[1], 8);
            if (imm_val == 0) {
                optimized = 1;  /* Eliminate the ADD instruction */
            } else {
                err = write_instruction(vm, &write_pc, &inst1);
                if (err != ERR_OK) return err;
            }
        }
        else {
            /* Write first instruction unchanged */
            err = write_instruction(vm, &write_pc, &inst1);
            if (err != ERR_OK) {
                return err;
            }
        }
        
        if (!optimized) {
            /* Write second instruction */
            err = write_instruction(vm, &write_pc, &inst2);
            if (err != ERR_OK) {
                return err;
            }
            pc = inst2.pc;
        } else {
            /* Skip both instructions */
            pc = inst2.pc;
        }
    }
    
    /* Copy HALT instruction */
    if (pc < POCOL_MEMORY_SIZE) {
        vm->memory[write_pc++] = vm->memory[pc];
    }
    
    return ERR_OK;
}

/* Main optimization function */
Err pocol_optimize_bytecode(PocolVM *vm, OptLevel level) {
    Err err = ERR_OK;
    
    switch (level) {
        case OPT_LEVEL_NONE:
            /* No optimization */
            break;
            
        case OPT_LEVEL_BASIC:
            /* Apply basic optimizations */
            err = pocol_opt_fold_constants(vm);
            if (err != ERR_OK) return err;
            
            err = pocol_opt_eliminate_dead_code(vm);
            if (err != ERR_OK) return err;
            break;
            
        case OPT_LEVEL_ADVANCED:
            /* Apply all optimizations */
            err = pocol_opt_fold_constants(vm);
            if (err != ERR_OK) return err;
            
            err = pocol_opt_eliminate_dead_code(vm);
            if (err != ERR_OK) return err;
            
            err = pocol_opt_peephole(vm);
            if (err != ERR_OK) return err;
            break;
    }
    
    return ERR_OK;
}