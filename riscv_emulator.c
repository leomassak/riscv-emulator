#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define ARCHLEN 32

#define RAM_SIZE 0b0010000000000000000

#define OPCODE_MASK 0b0000000000000000000000001111111
#define RD_MASK 0b0000000000000000000111110000000
#define RS1_MASK 0b0000000000011111000000000000000
#define RS2_MASK 0b0000001111100000000000000000000

uint8_t ram[RAM_SIZE];

uint32_t pc;
uint32_t next_pc;
uint32_t inst;
uint32_t reg[ARCHLEN];

uint32_t ram_start = 0;

unsigned char buffer[ARCHLEN];

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
            val = (int32_t)(val << (val2 & (ARCHLEN - 1)));
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
            val = (int32_t)((uint32_t)val >> (val2 & (ARCHLEN - 1)));
            break;
        case 5 | 8:
            //sra
            val = (int32_t)val >> (val2 & (ARCHLEN - 1));
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
        imm = ((inst & 0b11111111000000000000) << 11);
        if (rd != 0)
            reg[rd] = pc + 4;
        next_pc = (int32_t)(pc + imm);
        break;

    case 0b1100111: 
        //jalr
        imm = (int32_t)inst >> 20;
        val = pc + 4;
        next_pc = (int32_t)(reg[rs1] + imm);
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

    case 0b0000011: /* LOAD */

        funct3 = (inst >> 12) & 7;
        imm = (int32_t)inst >> 20;
        addr = reg[rs1] + imm;
        switch(funct3) {

        case 0: /* lb */
        {
            uint8_t rval;
    
            uint8_t* p = ram + addr;
            rval = p[0];
            val = (int8_t)rval;
        }
        break;

        case 1: /* lh */
        {
            uint16_t rval;
    
            uint16_t* p = ram + addr;
            rval = p[0];
            val = (int16_t)rval;
        }
        break;

        case 2: /* lw */
        {
            uint32_t rval;

            uint32_t* p = ram + addr;
            rval = p[0];
            val = (int32_t)rval;
        }
        break;

        case 4: /* lbu */
        {
             uint8_t rval;
    
            uint8_t* p = ram + addr;
            rval = p[0];
            val = rval;
        }
        break;

        case 5: /* lhu */
        {
            uint16_t rval;
    
            uint16_t* p = ram + addr;
            rval = p[0];
            val = rval;
        }
        break;

        default:
            print("Instrucao de Load nao identificada.");
            return;
        }
        if (rd != 0)
            reg[rd] = val;
        break;

    case 0b0100011: /* STORE */

        uint32_t h = inst & 0b111111100000000000000000000000;
        uint32_t l = inst & 0b000000000000000000000000011111;
                                                
        funct3 = (inst >> 12) & 7;
        imm = (h >> 18) + l;
        addr = reg[rs1] + imm;
        val = reg[rs2];
        switch(funct3) {

        case 0: /* sb */
            uint8_t* p = ram + addr;
            p[0] = val & 0b11111111;
            break;

        case 1: /* sh */
            uint8_t* p = ram + addr;
            p[0] = val & 0b11111111;
            p[1] = (val >> 8) & 0b11111111;
            break;

        case 2: /* sw */
            uint8_t* p = ram + addr;
            p[0] = val & 0b11111111;
            p[1] = (val >> 8) & 0b11111111;
            p[2] = (val >> 16) & 0b11111111;
            p[3] = (val >> 24) & 0b11111111;
            break;

        default:
            printf("Instrucao de store nao identificada.");
            return;
        }
        break;

    case 0b0010011: /* OP-IMM */

        funct3 = (inst >> 12) & 7;
        imm = (int32_t)inst >> 20;
        switch(funct3) {
        case 0: /* addi */
            val = (int32_t)(reg[rs1] + imm);
            break;
        case 1: /* slli */
            break;
        case 2: /* slti */
            val = (int32_t)reg[rs1] < (int32_t)imm;
            break;
        case 3: /* sltiu */
            val = reg[rs1] < (uint32_t)imm;
            break;
        case 4: /* xori */
            val = reg[rs1] ^ imm;
            break;
        case 5: /* ori */
            break;
        case 6: /* ori */
            val = reg[rs1] | imm;
            break;
        case 7: /* andi */
            val = reg[rs1] & imm;
            break;
        }
        if (rd != 0)
            reg[rd] = val;
        break;

    default:
        printf("Instrução não reconhecida");
        return;
    }
}


void riscv_decoder()
{
    while (1) {
        next_pc = pc + 4;

        inst = buffer;
        execute_instruction();

        pc = next_pc;
    }
}

int main(int argc, char** argv)
{
    FILE *ptr;

    ptr = fopen("test.bin","rb");

    
    if (!ptr) {
        perror("fopen");
        exit(EXIT_FAILURE);
    } 

    fread(buffer,sizeof(buffer),1,ptr);

    pc = ram_start;
    riscv_decoder();

    printf("Registradores:\n");
    for(int i = 0; i < ARCHLEN; i++) {
        printf("R[%i]: %i\n", i, reg[i]);
    }

    printf("\n");
    fclose(ptr);
    return 0;
}
