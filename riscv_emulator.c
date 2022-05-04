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
#define FUNCT7_MASK 0b11111110000000000000000000000000

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

uint32_t ram[RAM_SIZE] = {0b0};

uint32_t mem_pos = 0;

uint32_t last_instruction;
uint32_t pc;
uint32_t inst;
uint32_t reg[ARCHLEN] = {0b0};

void print_binary(unsigned int number)
{
    if (number >> 1) {
        print_binary(number >> 1);
    }
    putc((number & 1) ? '1' : '0', stdout);
}


void execute_instruction()
{
    uint32_t opcode, rd, rs1, rs2, funct3, funct7, val, val2;
    int32_t imm, cond;

    opcode = inst & OPCODE_MASK;
    rd = (inst & RD_MASK) >> 7;
    rs1 = (inst & RS1_MASK) >> 15;
    rs2 = (inst & RS2_MASK) >> 20;

    // printf("Opcode: %lu | rd: %lu | rs1: %lu | rs2: %lu\n\n", opcode, rd, rs1, rs2);

    switch(opcode) {
    case 0b0110011: 
        //OP
        val = reg[rs1];
        val2 = reg[rs2];
        funct3 = (inst & FUNCT3_MASK) >> 12;
        funct7 = (inst & FUNCT7_MASK) >> 25;

        switch(funct3) {
        case 0b000:
            if(funct7 == 0b0000000) {
                //add
                reg[rd] = (int32_t)(val + val2);
                printf("ADD x%lu x%lu x%lu\n", rd, rs2, rs1);
            } else if (funct7 == 0b0100000) {
                //sub
                reg[rd] = (int32_t)(val - val2);
                printf("SUB x%lu x%lu x%lu\n", rd, rs2, rs1);
            } else {
                printf("Operação não reconhecida\n");
            }
            break;
        case 0b001: 
            //sll
            reg[rd] = (int32_t)(val << (val2 & (ARCHLEN - 1)));
            printf("SLL\n");
            break;
        case 0b010: 
            //slt
            reg[rd] = (int32_t)val < (int32_t)val2;
            printf("SLT\n");
            break;
        case 0b011: 
            //sltu
            reg[rd] = val < val2;
            printf("SLTU\n");
            break;
        case 0b100: 
            //xor
            reg[rd] = val ^ val2;
            printf("XOR\n");
            break;
        case 0b101:
            if(funct7 == 0b0000000) {
                //srl
                reg[rd] = (int32_t)((uint32_t)val >> (val2 & (ARCHLEN - 1)));
                printf("SRL\n");
            } else if (funct7 == 0b0100000) {
                //sra
                reg[rd] = (int32_t)val >> (val2 & (ARCHLEN - 1));
                printf("SRA\n");
            } else {
                printf("Operação não reconhecida\n");
            }
            break;
        case 0b110: 
            //or
            reg[rd] = val | val2;
            printf("OR\n");
            break;
        case 0b111: 
            //and
            reg[rd] = val & val2;
            printf("AND\n");
            break;
        default:
            printf("Operação não reconhecida\n");
            return;
        }
        break;
    case 0b0110111:
        imm = inst & U_IMM_MASK; 
        //lui
        printf("LUI x%lu %lu\n", rd, imm);
        reg[rd] = (int32_t)(imm << 12);
        break;

    case 0b0010111: 
        //auipc
        printf("AUIPC x%lu %lu\n", rd, imm);
        reg[rd] = (int32_t)(pc + (int32_t)(imm << 12));
        break;

    case 0b1101111: 
        //jal
        imm = ((inst & IMM20_MASK) >> 11) + (inst & IMM19_12_MASK) + ((inst & J_IMM11_MASK) >> 9) + ((inst & IMM10_1_MASK) >> 20);
        printf("JAL X%lu %lu\n", rd, imm);
        reg[rd] = pc + 4;
        pc = (int32_t)(pc + imm);
        break;

    case 0b1100111: 
        //jalr
        printf("JARL\n");
        imm = (int32_t)(inst & OP_IMM_MASK) >> 20;
        printf("JARL X%lu %lu\n", rd, imm);
        pc = (int32_t)(reg[rs1] + imm) & ~1;
        reg[rd] = pc + 4;
        break;

    case 0b1100011: 
        // BRANCH
        funct3 = (inst & FUNCT3_MASK) >> 12;
        switch(funct3) {
        case 0b000:
            //beq
            printf("BEQ ");
            cond = (reg[rs1] == reg[rs2]);
            break;
        case 0b001:
            //bneq
            printf("BNE ");
            cond = (reg[rs1] != reg[rs2]);
            break;
        case 0b100: 
            //blt
            printf("BLT ");
            cond = ((int32_t)reg[rs1] < (int32_t)reg[rs2]);
            break;
        case 0b101: 
            //bge
            printf("BGE ");
            cond = ((int32_t)reg[rs1] >= (int32_t)reg[rs2]);
            break;
        case 0b110: 
            //bltu
            printf("BLTU ");
            cond = (reg[rs1] < reg[rs2]);
            break;
        case 0b111: 
            //bgeu
            printf("BGEU ");
            cond = (reg[rs1] >= reg[rs2]);
            break;
        default:
            printf("Instrução branch não reconhecida\n");
            return;
        }
        if (cond) {
            imm = ((inst & IMM12_MASK) >> 19) + ((inst & IMM11_MASK) << 4) + ((inst & IMM10_5_MASK) >> 20) + ((inst & IMM4_1_MASK) >> 7);
            if((imm & 0b1000000000000) >> 12 == 1) {
              imm = ~(~imm & 0b0111111111111);
            }

            printf("x%lu x%lu %lu\n", rs1, rs2, imm);
            pc = (uint32_t)(pc + (imm/4)-1);
            break;
        } else {
            printf("x%lu x%lu %lu\n", rs1, rs2, imm);
        }
        break;

    case 0b0000011: /* LOAD */

        funct3 = (inst & FUNCT3_MASK) >> 12;
        imm = (int32_t)inst >> 20;
        switch(funct3) {

        case 0b000: /* lb */
        {
          printf("LB\n");
          val = (int8_t)ram[reg[rs1] & 0b00000000000000000000000011111111];
        }
        break;

        case 0b001: /* lh */
        {
          printf("LH\n");
          val = (int16_t)ram[reg[rs1] & 0b00000000000000001111111111111111];
        }
        break;

        case 0b010: /* lw */
        {
          printf("LW\n");
          val = (int32_t)ram[reg[rs1]];
        }
        break;

        case 0b100: /* lbu */
        {
          printf("LBU\n");
          val = (uint8_t)ram[reg[rs1] & 0b00000000000000000000000011111111];
        }
        break;

        case 0b101: /* lhu */
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

        funct3 = (inst & FUNCT3_MASK) >> 12;
        imm = ((inst & IMM11_5_MASK) >> 20) + (inst & IMM4_0_MASK) >> 7;
        val = reg[rs2];
        switch(funct3) {

        case 0b000: /* sb */
            printf("SB\n");
            ram[reg[rs1]] = (int8_t)(reg[rs2] & 0b00000000000000000000000011111111);
            break;

        case 0b001: /* sh */
            printf("SH\n");
            ram[reg[rs1]] = (int16_t)(reg[rs2] & 0b00000000000000001111111111111111);
            break;

        case 0b010: /* sw */
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

        funct3 = (inst & FUNCT3_MASK) >> 12;
        imm = (inst & OP_IMM_MASK) >> 20;
        if((imm & 0b100000000000) >> 11 == 1) {
              imm = ~(~imm & 0b0111111111111);
            }

        switch(funct3) {
        case 0b000: /* addi */
            printf("ADDI x%lu x%lu %lu\n", rd, rs1, imm);
            reg[rd] = (int32_t)(reg[rs1] + imm);
            break;
        case 0b001: /* slli */
            printf("SLLI\n");
            reg[rd] = (int32_t)reg[rs1] << (int32_t)reg[imm];
            break;
        case 0b010: /* slti */
            printf("SLTI\n");
            reg[rd] = (int32_t)reg[rs1] < (int32_t)imm;
            break;
        case 0b011: /* sltiu */
            printf("SLTIU\n");
            reg[rd] = reg[rs1] < (uint32_t)imm;
            break;
        case 0b100: /* xori */
            printf("XORI\n");
            reg[rd] = reg[rs1] ^ imm;
            break;
        case 0b101: /* srli */
            reg[rd] = (uint8_t)reg[rs1] >> imm;
            break;
        case 0b110: /* ori */
            printf("ORI\n");
            reg[rd] = reg[rs1] | imm;
            break;
        case 0b111: /* andi */
            printf("ANDI\n");
            reg[rd] = reg[rs1] & imm;
            break;
        }
        break;

    default:
        printf("Instrução não reconhecida\n");
        return;
    }
}


void riscv_decoder()
{
    char* binary;
    while(pc <= last_instruction) {
      inst = ram[pc];
      pc = pc + 1;

      printf("Instruction: ");
      print_binary(inst);
      printf("\n");
      execute_instruction();

      printf("pc -> %d\n\n", pc);
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
            fread(&ram[0],sizeof(inst),1,fl);
        } else {
            fread(&ram[mem_pos],sizeof(inst),1,fl);
        }
        mem_pos += 1;
    }

    printf("mem_pos: %d\n", mem_pos);
    printf("pc init: %d\n\n", pc);

    printf("RAM:\n");
    for(int i = 0; i < ARCHLEN; i++) {
        if(ram[i]) {
            printf("RAM[%i]: %i\n", i, ram[i]);
        }
    }
    printf("\n");

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
