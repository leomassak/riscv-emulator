#include <stdio.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define OPCODEMASK 00000000000000000000000001111111
#define FUNCT7_MASK 11111110000000000000000000000000
#define RS1_MASK 00000000000011111000000000000000
#define RS2_MASK 00000001111100000000000000000000
#define FUNCT3_MASK 00000000000000000111000000000000
#define RD_MASK 00000000000000000000111110000000

// CPU state
uint32_t pc;
uint32_t next_pc;
uint32_t insn;
uint32_t regs[32];

void parse_instruction() {

}

int main(int argc, char** argv){

    uint32_t opcode, funct7, rs1,rs2, funct3, rd;
    char instruction, inst_type;

    // instruction type reading
    opcode = instruction & OPCODEMASK;

    sswitch(opcode) {
        case 0110011:
            //R
            //decodeRtype();
            funct7 = (inst & rc.FUNCT7_MASK) >> 25
            rs1    = (inst & rc.RS1_MASK)    >> 15
            rs2    = (inst & rc.RS2_MASK)    >> 20
            funct3 = (inst & rc.FUNCT3_MASK) >> 12
            rd     = (inst & rc.RD_MASK)     >> 7
            break;
        case 0010011:
            //I
            decodeRtype();
            break;
        case 0100011:
            //S
            decodeRtype();
            break;
        case 1100011:
            //B
            decodeRtype();
            break;
        case 01101111:
            //J
            decodeRtype();
            break;
        case 0110111:
        case 0010111:
        case 1101111:
            //U
            decodeRtype();
            break;
        default:
            printf("Instrução desconhecida.");

    }
    // command reading

    return 0;
}