#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define ARCHLEN 32

#define RAM_SIZE 0b1000000000000

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

uint8_t ram[RAM_SIZE] = {0b0};

uint32_t mem_pos = 0;

uint32_t last_instruction;
uint32_t pc;
uint32_t next_pc;
uint32_t inst;
uint32_t reg[ARCHLEN] = {0b0};


void execute_instruction()
{
    uint32_t opcode, rd, rs1, rs2, funct3, val=0, val2;
    int32_t imm, cond;

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
            printf("ADD\n");
            break;
        case 0 | 8: 
            //sub
            val = (int32_t)(val - val2);
            printf("SUB\n");
            break;
        case 1: 
            //sll
            val = (int32_t)(val << (val2 & (ARCHLEN - 1)));
            printf("SLL\n");
            break;
        case 2: 
            //slt
            val = (int32_t)val < (int32_t)val2;
            printf("SLT\n");
            break;
        case 3: 
            //sltu
            val = val < val2;
            printf("SLTU\n");
            break;
        case 4: 
            //xor
            val = val ^ val2;
            printf("XOR\n");
            break;
        case 5: 
            //srl
            val = (int32_t)((uint32_t)val >> (val2 & (ARCHLEN - 1)));
            printf("SRL\n");
            break;
        case 5 | 8:
            //sra
            val = (int32_t)val >> (val2 & (ARCHLEN - 1));
            printf("SRA\n");
            break;
        case 6: 
            //or
            val = val | val2;
            printf("OR\n");
            break;
        case 7: 
            //and
            val = val & val2;
            printf("AND\n");
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
        printf("LUI\n");
        if (rd != 0)
            reg[rd] = (int32_t)(imm << 12);
        break;

    case 0b0010111: 
        //auipc
        printf("AUIPC\n");
        if (rd != 0)
            reg[rd] = (int32_t)(pc + (int32_t)(imm << 12));
        break;

    case 0b1101111: 
        //jal
        printf("JAL\n");
        imm = ((inst & IMM20_MASK) >> 11) + (inst & IMM19_12_MASK) + ((inst & J_IMM11_MASK) >> 9) + ((inst & IMM10_1_MASK) >> 20);
        if (rd != 0)
            reg[rd] = pc + 4;
        next_pc = (int32_t)(pc + imm);
        break;

    case 0b1100111: 
        //jalr
        printf("JARL\n");
        imm = (int32_t)(inst & OP_IMM_MASK) >> 20;
        val = pc + 4;
        next_pc = (int32_t)(reg[rs1] + imm) & ~1;
        if (rd != 0)
            reg[rd] = val;
        break;

    case 0b1100011: 
        // BRANCH
        funct3 = (inst & FUNCT3_MASK) >> 12;
        switch(funct3) {
        case 0b000:
            //beq
            printf("BEQ");
            cond = (reg[rs1] == reg[rs2]);
            printf("Cond saindo: %d\n", cond);
            break;
        case 0b001:
            //bneq
            printf("BNE\n");
            cond = (reg[rs1] != reg[rs2]);
            printf("Cond saindo: %d\n", cond);
            break;
        case 0b100: 
            //blt
            printf("BLT\n");
            cond = ((int32_t)reg[rs1] < (int32_t)reg[rs2]);
            break;
        case 0b101: 
            //bge
            printf("BGE\n");
            cond = ((int32_t)reg[rs1] >= (int32_t)reg[rs2]);
            break;
        case 0b110: 
            //bltu
            printf("BLTU\n");
            cond = (reg[rs1] < reg[rs2]);
            break;
        case 0b111: 
            //bgeu
            printf("BGEU\n");
            cond = (reg[rs1] >= reg[rs2]);
            break;
        default:
            printf("Instrução branch não reconhecida\n");
            return;
        }
        // cond ^= (funct3 & 1);
        if (cond) {
            imm = ((inst & IMM12_MASK) >> 19) + ((inst & IMM11_MASK) << 4) + ((inst & IMM10_5_MASK) >> 20) + ((inst & IMM4_1_MASK) >> 7);
            

            next_pc = (int32_t)(pc + imm);
            break;
        }
        break;

    case 0b0000011: /* LOAD */

        funct3 = (inst >> 12) & 7;
        imm = (int32_t)inst >> 20;
        switch(funct3) {

        case 0: /* lb */
        {
          printf("LB\n");
          val = (int8_t)ram[reg[rs1] & 0b00000000000000000000000011111111];
        }
        break;

        case 1: /* lh */
        {
          printf("LH\n");
          val = (int16_t)ram[reg[rs1] & 0b00000000000000001111111111111111];
        }
        break;

        case 2: /* lw */
        {
          printf("LW\n");
          val = (int32_t)ram[reg[rs1]];
        }
        break;

        case 4: /* lbu */
        {
          printf("LBU\n");
          val = (uint8_t)ram[reg[rs1] & 0b00000000000000000000000011111111];
        }
        break;

        case 5: /* lhu */
        {
          printf("LHU\n");
          val = (uint16_t)ram[reg[rs1] & 0b00000000000000001111111111111111];
        }
        break;

        default:
            printf("Instrucao de Load nao identificada.\n");
            return;
        }
        if (rd != 0)
            reg[rd] = val;
        break;

    case 0b0100011: /* STORE */

        funct3 = (inst >> 12) & 7;
        imm = ((inst & IMM11_5_MASK) >> 20) + (inst & IMM4_0_MASK) >> 7;
        val = reg[rs2];
        switch(funct3) {

        case 0: /* sb */
            printf("SB\n");
            ram[reg[rs1]] = (int8_t)(reg[rs2] & 0b00000000000000000000000011111111);
            break;

        case 1: /* sh */
            printf("SH\n");
            ram[reg[rs1]] = (int16_t)(reg[rs2] & 0b00000000000000001111111111111111);
            break;

        case 2: /* sw */
            printf("SW\n");
            if((imm & 0b100000000000) >> 11 == 1) {
              uint32_t two_complement = ~imm & 0b0111111111111;
              imm = ~two_complement;
            }
            ram[reg[rs1] + imm] = (int32_t)(reg[rs2]);
            break;

        default:
            printf("Instrucao de store nao identificada.\n");
            return;
        }
        break;

    case 0b0010011: /* OP-IMM */

        funct3 = (inst >> 12) & 7;
        imm = (inst & OP_IMM_MASK) >> 20;
        if((imm & 0b100000000000) >> 11 == 1) {
              uint32_t two_complement = ~imm & 0b0111111111111;
              imm = ~two_complement;
        }

        switch(funct3) {
        case 0: /* addi */
            printf("ADDI\n");
            val = (int32_t)(reg[rs1] + imm);
            break;
        case 1: /* slli */
            printf("SLLI\n");
            val = (int32_t)reg[rs1] << (int32_t)reg[imm];
            break;
        case 2: /* slti */
            printf("SLTI\n");
            val = (int32_t)reg[rs1] < (int32_t)imm;
            break;
        case 3: /* sltiu */
            printf("SLTIU\n");
            val = reg[rs1] < (uint32_t)imm;
            break;
        case 4: /* xori */
            printf("XORI\n");
            val = reg[rs1] ^ imm;
            break;
        case 5: /* srli/srai */
            break;
        case 6: /* ori */
            printf("ORI\n");
            val = reg[rs1] | imm;
            break;
        case 7: /* andi */
            printf("ANDI\n");
            val = reg[rs1] & imm;
            break;
        }
        if (rd != 0)
            reg[rd] = val;
        break;

    default:
        printf("Instrução não reconhecida\n");
        return;
    }
}


void riscv_decoder()
{
    while(pc <= last_instruction) {
      // inst = 0x02208133;
      inst = ram[pc];
      next_pc = pc + 1;

      // printf("insstruction: %lu", inst);
      execute_instruction();

      pc = next_pc;
      printf("pc instruction end: %d\n", pc);
  }
}

int main(int argc, char** argv)
{ 
    pc = -1;
    FILE *fl;

    fl = fopen("prog.bin","rb");

    
    if (!fl) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    while (!feof(fl)){     
        if(pc == -1) {
            pc = 0;
            next_pc = 0;
            fread(&ram[0],4,1,fl);
        } else {
            fread(&ram[mem_pos],4,1,fl);
        }
        mem_pos += 1;
    }

    printf("mem_pos: %d\n", mem_pos);
    printf("pc init: %d\n", pc);

    last_instruction = mem_pos - 1;

    riscv_decoder();
    printf("Registradores:\n");
    for(int i = 0; i < ARCHLEN; i++) {
        printf("R[%i]: %i\n", i, reg[i]);
    }

    printf("\n");
    fclose(fl);
    return 0;
}
