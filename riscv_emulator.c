#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define ARCHLEN 32

#define RAM_SIZE 0b0010000000000000000

//COMMON MASKS
#define OPCODE_MASK 0b00000000000000000000000001111111
#define RD_MASK 0b00000000000000000000111110000000
#define RS1_MASK 0b00000000000011111000000000000000
#define RS2_MASK 0b00000001111100000000000000000000
#define FUNCT3_MASK 0b00000000000000000111000000000000

//OP-IMM MASKS
#define OP_IMM_MASK 0b11111111111100000000000000000000

//STORE MASKS
#define IMM4_0_MASK 0b00000000000000000000111110000000
#define IMM11_5_MASK 0b11111110000000000000000000000000

//BRANCH MASKS
#define IMM11_MASK 0b00000000000000000000000010000000
#define IMM4_1_MASK 0b00000000000000000000111100000000
#define IMM10_5_MASK 0b01111110000000000000000000000000
#define IMM12_MASK 0b10000000000000000000000000000000

//U-TYPE MASKS
#define U_IMM_MASK 0b11111111111111111111000000000000

//J-TYPE MASKS
#define IMM19_12_MASK 0b00000000000011111111000000000000
#define J_IMM11_MASK 0b00000000000100000000000000000000
#define IMM10_1_MASK 0b01111111111000000000000000000000
#define IMM20_MASK 0b10000000000000000000000000000000

uint8_t ram[RAM_SIZE];

uint32_t mem_pos = 0;

uint32_t last_instruction;
uint32_t pc;
uint32_t next_pc;
uint32_t inst;
uint32_t reg[ARCHLEN] = {0b0};

unsigned char buffer[ARCHLEN];

void execute_instruction()
{
    uint32_t opcode, rd, rs1, rs2, funct3;
    int32_t imm, cond;
    uint32_t addr, val=0, val2;

    opcode = inst & OPCODE_MASK;
    rd = (inst & RD_MASK) >> 7;
    rs1 = (inst & RS1_MASK) >> 15;
    rs2 = (inst & RS2_MASK) >> 20;

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
        imm = inst & U_IMM_MASK; 
        //lui
        if (rd != 0)
            reg[rd] = (int32_t)(imm << 12);
        break;

    case 0b0010111: 
        //auipc
        if (rd != 0)
            reg[rd] = (int32_t)(pc + (int32_t)(imm << 12));
        break;

    case 0b1101111: 
        //jal
        imm = ((inst & IMM20_MASK) >> 11) + (inst & IMM19_12_MASK) + ((inst & J_IMM11_MASK) >> 9) + ((inst & IMM10_1_MASK) >> 20);
        if (rd != 0)
            reg[rd] = pc + 4;
        next_pc = (int32_t)(pc + imm);
        break;

    case 0b1100111: 
        //jalr
        imm = (int32_t)(inst & OP_IMM_MASK) >> 20;
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
            imm = ((inst & IMM12_MASK) >> 19) + ((inst & IMM11_MASK) << 4) + ((inst & IMM10_5_MASK) >> 20) + ((inst & IMM4_1_MASK) >> 7);
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
            val = (int8_t)ram[reg[rs1] & 0b00000000000000000000000011111111];
        }
        break;

        case 1: /* lh */
        {
            val = (int16_t)ram[reg[rs1] & 0b00000000000000001111111111111111];
        }
        break;

        case 2: /* lw */
        {
            val = (int32_t)ram[reg[rs1]];
        }
        break;

        case 4: /* lbu */
        {
            val = (uint8_t)ram[reg[rs1] & 0b00000000000000000000000011111111];
        }
        break;

        case 5: /* lhu */
        {
            val = (uint16_t)ram[reg[rs1] & 0b00000000000000001111111111111111];
        }
        break;

        default:
            printf("Instrucao de Load nao identificada.");
            return;
        }
        if (rd != 0)
            reg[rd] = val;
        break;

    case 0b0100011: /* STORE */

        funct3 = (inst >> 12) & 7;
        imm = ((inst & IMM11_5_MASK) >> 20) + (inst & IMM4_0_MASK) >> 7;
        addr = reg[rs1] + imm;
        val = reg[rs2];
        switch(funct3) {

        case 0: /* sb */
            ram[reg[rs1]] = (int8_t)(reg[rs2] & 0b00000000000000000000000011111111);
            break;

        case 1: /* sh */
            ram[reg[rs1]] = (int16_t)(reg[rs2] & 0b00000000000000001111111111111111);
            break;

        case 2: /* sw */
            if((imm & 0b100000000000) >> 11 == 1) {
              uint32_t two_complement = ~imm & 0b0111111111111;
              imm = ~two_complement;
            }
            ram[reg[rs1] + imm] = (int32_t)(reg[rs2]);
            break;

        default:
            printf("Instrucao de store nao identificada.");
            return;
        }
        break;

    case 0b0010011: /* OP-IMM */

        funct3 = (inst >> 12) & 7;
        imm = (inst & OP_IMM_MASK) >> 20;
        switch(funct3) {
        case 0: /* addi */
            val = (int32_t)(reg[rs1] + imm);
            break;
        case 1: /* slli */
            val = (int32_t)reg[rs1] << (int32_t)reg[imm];
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
    while(pc <= last_instruction) {
      if(pc != 0) {
        next_pc = pc + 4;
      }

      inst = ram[pc];
      execute_instruction();

      pc = next_pc;
  }
}

int main(int argc, char** argv)
{
    pc = -1;
    FILE *ptr;

    ptr = fopen("prog.bin","rb");

    
    if (!ptr) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    while (!feof(ptr)){     
        if(pc == -1) {
            pc = 0;
            next_pc = 0;
            fread(&ram[0],4,1,ptr);
        } else {
            fread(&ram[mem_pos],4,1,ptr);
        }
        mem_pos += 1;
    }

    last_instruction = mem_pos - 1;

    riscv_decoder();
    printf("Registradores:\n");
    for(int i = 0; i < ARCHLEN; i++) {
        printf("R[%i]: %i\n", i, reg[i]);
    }

    printf("\n");
    fclose(ptr);
    return 0;
}
