#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define XLEN 32

#define RAM_SIZE 0b0010000000000000000

#define OPCODE_MASK 0b0000000000000000000000001111111
#define RD_MASK 0b0000000000000000000111110000000
#define RS1_MASK 0b0000000000011111000000000000000
#define RS2_MASK 0b0000001111100000000000000000000

uint8_t ram[RAM_SIZE];

uint32_t pc;
uint32_t next_pc;
uint32_t inst;
uint32_t reg[32];

uint32_t ram_start;

uint64_t inst_count = 0;


int target_read_u8(uint8_t *pval, uint32_t addr)
{
    addr -= ram_start;
    if (addr > RAM_SIZE) {
        *pval = 0;
        printf("illegal read 8, PC: 0x%08x, address: 0x%08x\n", pc, addr + ram_start);
        return 1;
    } else {
        uint8_t* p = ram + addr;
        *pval = p[0];
    }
    return 0;
}

uint32_t get_inst(uint32_t pc)
{
    uint32_t ptr = pc - ram_start;
    if (ptr > RAM_SIZE) return 1;
    uint8_t* p = ram + ptr;
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}


void execute_instruction()
{
    uint32_t opcode, rd, rs1, rs2, funct3;
    int32_t imm, cond;
    uint32_t addr, val=0, val2;

    opcode = inst & OPCODE_MASK;
    rd = (inst >> 7) & RD_MASK;
    rs1 = (inst >> 15) & RS1_MASK;
    rs2 = (inst >> 20) & RS2_MASK;

    switch(opcode) {

    case 0b0110011: 
        //OP
        imm = inst >> 25;
        val = reg[rs1];
        val2 = reg[rs2];
        funct3 = ((inst >> 12) & 7) | ((inst >> (30 - 3)) & (1 << 3));
        switch(funct3) {
        case 0: 
            //add
            val = (int32_t)(val + val2);
            break;
        case 0 | 8: 
            //sub
            val = (int32_t)(val - val2);
            break;
        case 1: 
            //sll
            val = (int32_t)(val << (val2 & (XLEN - 1)));
            break;
        case 2: 
            //slt
            val = (int32_t)val < (int32_t)val2;
            break;
        case 3: 
            //sltu
            val = val < val2;
            break;
        case 4: 
            //xor
            val = val ^ val2;
            break;
        case 5: 
            //srl
            val = (int32_t)((uint32_t)val >> (val2 & (XLEN - 1)));
            break;
        case 5 | 8:
            //sra
            val = (int32_t)val >> (val2 & (XLEN - 1));
            break;
        case 6: 
            //or
            val = val | val2;
            break;
        case 7: 
            //and
            val = val & val2;
            break;
        default:
            printf("Operação não reconhecida\n");
            return;
        }
        if (rd != 0)
            reg[rd] = val;
        break;

    case 0b0110111: 
        //lui
        if (rd != 0)
            reg[rd] = (int32_t)(inst & 0b11111111111111111111000000000000);
        break;

    case 0b0010111: 
        //auipc
        if (rd != 0)
            reg[rd] = (int32_t)(pc + (int32_t)(inst & 0b11111111111111111111000000000000));
        break;

    case 0b1101111: 
        //jal
        imm = ((inst >> (31 - 20)) & (1 << 20)) |
              ((inst >> (21 - 1)) & 0b11111111110) |
              ((inst >> (20 - 11)) & (1 << 11)) |
              (inst & 0xff000);
        imm = (imm << 11) >> 11;
        if (rd != 0)
            reg[rd] = pc + 4;
        next_pc = (int32_t)(pc + imm);
        break;

    case 0b1100111: 
        //jalr
        imm = (int32_t)inst >> 20;
        val = pc + 4;
        next_pc = (int32_t)(reg[rs1] + imm) & ~1;
        if (rd != 0)
            reg[rd] = val;
        break;

    case 0b1100011: 
        // BRANCH
        funct3 = (inst >> 12) & 7;
        switch(funct3 >> 1) {
        case 0:
            //beq/bne
            cond = (reg[rs1] == reg[rs2]);
            break;
        case 2: 
            //blt/bge
            cond = ((int32_t)reg[rs1] < (int32_t)reg[rs2]);
            break;
        case 3: 
            //bltu/bgeu
            cond = (reg[rs1] < reg[rs2]);
            break;
        default:
            printf("Instrução branch não reconhecida");
            return;
        }
        cond ^= (funct3 & 1);
        if (cond) {
            imm = ((inst >> (31 - 12)) & (1 << 12)) |
                  ((inst >> (25 - 5)) & 0b11111100000) |
                  ((inst >> (8 - 1)) & 0b0011110) |
                  ((inst << (11 - 7)) & (1 << 11));
            imm = (imm << 19) >> 19;
            next_pc = (int32_t)(pc + imm);
            break;
        }
        break;

    // case 0b0000011: /* LOAD */

    //     funct3 = (inst >> 12) & 7;
    //     imm = (int32_t)inst >> 20;
    //     addr = reg[rs1] + imm;
    //     switch(funct3) {

    //     case 0: /* lb */
    //     {
    //         uint8_t rval;
    //         if (target_read_u8(&rval, addr)) {
    //             printf("Endereco de memoria nao suportado");
    //             return;
    //         }
    //         val = (int8_t)rval;
    //     }
    //     break;

    //     case 1: /* lh */
    //     {
    //         uint16_t rval;
    //         if (target_read_u16(&rval, addr)) {
    //             raise_exception(pending_exception, pending_tval);
    //             return;
    //         }
    //         val = (int16_t)rval;
    //     }
    //     break;

    //     case 2: /* lw */
    //     {
    //         uint32_t rval;
    //         if (target_read_u32(&rval, addr)) {
    //             raise_exception(pending_exception, pending_tval);
    //             return;
    //         }
    //         val = (int32_t)rval;
    //     }
    //     break;

    //     case 4: /* lbu */
    //     {
    //         uint8_t rval;
    //         if (target_read_u8(&rval, addr)) {
    //             raise_exception(pending_exception, pending_tval);
    //             return;
    //         }
    //         val = rval;
    //     }
    //     break;

    //     case 5: /* lhu */
    //     {
    //         uint16_t rval;
    //         if (target_read_u16(&rval, addr)) {
    //             raise_exception(pending_exception, pending_tval);
    //             return;
    //         }
    //         val = rval;
    //     }
    //     break;

    //     default:
    //         raise_exception(CAUSE_ILLEGAL_INSTRUCTION, inst);
    //         return;
    //     }
    //     if (rd != 0)
    //         reg[rd] = val;
    //     break;

    // case 0b0100011: /* STORE */

    //     funct3 = (inst >> 12) & 7;
    //     imm = rd | ((inst >> (25 - 5)) & 0xfe0);
    //     imm = (imm << 20) >> 20;
    //     addr = reg[rs1] + imm;
    //     val = reg[rs2];
    //     switch(funct3) {

    //     case 0: /* sb */
    //         if (target_write_u8(addr, val)) {
    //             raise_exception(pending_exception, pending_tval);
    //             return;
    //         }
    //         break;

    //     case 1: /* sh */
    //         if (target_write_u16(addr, val)) {
    //             raise_exception(pending_exception, pending_tval);
    //             return;
    //         }
    //         break;

    //     case 2: /* sw */
    //         if (target_write_u32(addr, val)) {
    //             raise_exception(pending_exception, pending_tval);
    //             return;
    //         }
    //         break;

    //     default:
    //         raise_exception(CAUSE_ILLEGAL_INSTRUCTION, inst);
    //         return;
    //     }
    //     break;

    // case 0b0010011: /* OP-IMM */

    //     funct3 = (inst >> 12) & 7;
    //     imm = (int32_t)inst >> 20;
    //     switch(funct3) {
    //     case 0: /* addi */
    //         val = (int32_t)(reg[rs1] + imm);
    //         break;
    //     case 1: /* slli */
    //         if ((imm & ~(XLEN - 1)) != 0) {
    //             raise_exception(CAUSE_ILLEGAL_INSTRUCTION, inst);
    //             return;
    //         }
    //         val = (int32_t)(reg[rs1] << (imm & (XLEN - 1)));
    //         break;
    //     case 2: /* slti */
    //         val = (int32_t)reg[rs1] < (int32_t)imm;
    //         break;
    //     case 3: /* sltiu */
    //         val = reg[rs1] < (uint32_t)imm;
    //         break;
    //     case 4: /* xori */
    //         val = reg[rs1] ^ imm;
    //         break;
    //     case 5: /* srli/srai */
    //         if ((imm & ~((XLEN - 1) | 0x400)) != 0) {
    //             raise_exception(CAUSE_ILLEGAL_INSTRUCTION, inst);
    //             return;
    //         }
    //         if (imm & 0x400)
    //         {
    //             val = (int32_t)reg[rs1] >> (imm & (XLEN - 1));
    //         }
    //         else
    //         {
    //             val = (int32_t)((uint32_t)reg[rs1] >> (imm & (XLEN - 1)));
    //         }
    //         break;
    //     case 6: /* ori */
    //         val = reg[rs1] | imm;
    //         break;
    //     case 7: /* andi */
    //         val = reg[rs1] & imm;
    //         break;
    //     }
    //     if (rd != 0)
    //         reg[rd] = val;
    //     break;

    default:
        printf("Instrução não reconhecida");
        return;
    }
}


void riscv_decoder()
{
    while (1) {
        next_pc = pc + 4;

        inst = get_inst(pc);
        inst_count++;
        execute_instruction();

        pc = next_pc;
    }
}

int main(int argc, char** argv)
{

    /* automatic STDOUT flushing, no fflush needed */
    setvbuf(stdout, NULL, _IONBF, 0);

    /* run program in emulator */
    pc = ram_start;
    riscv_decoder();

    printf("\n");
    printf(">>> Instruction count: %llu\n",(long long unsigned)inst_count);
    // printf(">>> Jumps: %llu (%2.2lf%%) - %llu forwards, %llu backwards\n",(long long unsigned)jump_counter,jump_counter*100.0/insn_counter,(long long unsigned)forward_counter,(long long unsigned)backward_counter);
    // printf(">>> Branching T=%llu (%2.2lf%%) F=%llu (%2.2lf%%)\n",(long long unsigned)true_counter,true_counter*100.0/(true_counter+false_counter),(long long unsigned)false_counter,false_counter*100.0/(true_counter+false_counter));
    printf("\n");
    return 0;
}
