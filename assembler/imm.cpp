#include <cstdio>
#include <vector>
#include <iostream>
#include "parsing.h"

enum opcode {
    AND = 0x00,
    OR = 0x01,
    XOR = 0x02,
    NOT = 0x03,
    ADD = 0x04,
    ADC = 0x05,
    SUB = 0x06,
    MULT = 0x07,
    DIV = 0x08,
    CMP = 0x09,
    XCHG = 0x0a,

    MOV = 0x0b,
    LOAD = 0x0c,
    STORE = 0x0d,
    PUSH = 0x0e,
    POP = 0x0f,

    MOVIU = 0x10,
    ADDI = 0x11,
    ADDIU = 0x12,
    CMPIU = 0x13,
    SHIFT = 0x14,

    JL = 0x15,
    JNE = 0x16,
    JR = 0x17,
    JSR = 0x18,
    UNUSED_19 = 0x19,
    JMP = 0x1a,

    SYS = 0x1b,
    SWAPCC = 0x1c,
    UNUSED_1d = 0x1d,
    UNUSED_1e = 0x1e,
    HALT = 0x1f,
};

typedef unsigned int uint;
int main()
{
    int l = 1;

    printf("signed relative 27 bits, shift 2\n");
    {
        uint addr = 0x100;
        std::vector<uint> values = {0, 1, addr + 0, addr + 0x100, 1 << 28, 1 << 29, (1 << 29) + addr};
        for(uint& u : values) {
            printf("%08X -> %08X\n", u, instr_infos[opcode::JL].Encode(u, l++, addr));
        }
    }
    {
        uint addr = (1 << 29) + 4;
        std::vector<uint> values = {0};
        for(uint& u : values) {
            printf("%08X -> %08X\n", u, instr_infos[opcode::JL].Encode(u, l++, addr));
        }
    }

    printf("signed relative 24 bits, shift 2\n");

    {
        uint addr = 0x100;
        std::vector<uint> values = {0, 1, addr + 0, addr + 0x100, 1 << 25, 1 << 26, (1 << 26) + addr};
        for(uint& u : values) {
            printf("%08X -> %08X\n", u, instr_infos[opcode::JSR].Encode(u, l++, addr));
        }
    }
    {
        uint addr = (1 << 26) + 4;
        std::vector<uint> values = {0};
        for(uint& u : values) {
            printf("%08X -> %08X\n", u, instr_infos[opcode::JSR].Encode(u, l++, addr));
        }
    }

    printf("signed 24 bits\n");
    {
        std::vector<uint> values = {0, 1, 1 << 24, (uint)(-1), (uint)(-(1 << 24) + 1), (uint)(-(1 << 24) - 1)};
        for(uint& u : values) {
            printf("%08X -> %08X\n", u, instr_infos[opcode::ADDI].Encode(u, l++, 0));
        }
    }

    printf("unsigned 24 bits\n");
    {
        std::vector<uint> values = {0, 1, (1 << 24) - 1, 1 << 24, };
        for(uint& u : values) {
            printf("%08X -> %08X\n", u, instr_infos[opcode::ADDIU].Encode(u, l++, 0));
        }
    }

    printf("unsigned 16 bits\n");
    {
        std::vector<uint> values = {0, 1, (1 << 16) - 1, 1 << 16};
        for(uint& u : values) {
            printf("%08X -> %08X\n", u, instr_infos[opcode::MOVIU].Encode(u, l++, 0));
        }
    }
}
