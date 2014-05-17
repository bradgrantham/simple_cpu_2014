#include <cassert>
#include <cstdio>
#include <cstring>
#include "simple_cpu_2014.hpp"
#include "util.hpp"

void write_memtest_program(memory& m, uint32_t base)
{
    using namespace simple_cpu_2014;

    uint32_t a = base;

    auto append = [&] (uint32_t data) {
        m.w(a, data);
        a += 4;
    };

    uint32_t memory_test_start = 0x00000200;
    uint32_t memory_test_end = 0x00800000;

    // Program starts at 0x000000h
    append(MOVIU(reg::SP, 0));
    append(ADDIU(reg::SP, 0x800000)); // Stack starts at 0x800000

    append(MOVIU(reg::R0, 0x000000)); // p test start a = 0x000100
    append(ADDI(reg::R0, memory_test_start));
    append(MOVIU(reg::R1, 0x000000)); // data
    append(MOVIU(reg::R5, 0x00FFFF)); // mask for data to keep it 16 bits
    append(MOVIU(reg::R3, 0x000100)); // test “shift” (R3 should be 0x2000000 after next instr)
    append(SHIFT(reg::R3, 0x000001));

    uint32_t loop = a;
    append(STORE(reg::R0, reg::R1, opsize::SIZE_16, 0x000000));
    append(LOAD(reg::R2, reg::R0, opsize::SIZE_16, 0x000000));
    append(CMP(reg::R2, reg::R1, opsize::SIZE_32, 0x000000));
    uint32_t jump_out_addr = a;
    append(0); // patch later with JNE(64 / 4));

    append(ADDI(reg::R0, 2)); // +2 for 16-bit word a, XXX signed operand
    append(ADDI(reg::R1, 2)); // XXX signed operand
    append(AND(reg::R1, reg::R5, opsize::SIZE_32, 0x000000)); // XXX signed operand
    append(CMPIU(reg::R0, memory_test_end)); // should be 0x800000
    append(JL((loop - a) / 4)); // XXX signed operand
    append(MOVIU(reg::R3, 0x1234)); // data
    append(ADDI(reg::R3, 0x5678)); // data
    append(PUSH(reg::R3, 0)); // data
    append(POP(reg::R4, 0)); // data
    append(STORE(reg::R5, reg::R3, opsize::SIZE_16, 0x000000));
    append(LOAD(reg::R6, reg::R5, opsize::SIZE_16, 0x000000));

    append(JMP((a + 8) / 4)); // skip the next dword, which is an inline literal
    append(0xf0000000); // console putchar address
    append(LOAD(reg::R1, reg::PC, opsize::SIZE_32, -4)); // R1 = console putchar addr
    append(MOVIU(reg::R2, 'S'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(MOVIU(reg::R2, 'u'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(MOVIU(reg::R2, 'c'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(MOVIU(reg::R2, 'c'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(MOVIU(reg::R2, 'e'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(MOVIU(reg::R2, 's'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(MOVIU(reg::R2, 's'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(MOVIU(reg::R2, '\n'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(HALT(0x0));

    // patch failure jump to here
    m.w(jump_out_addr, JNE((a - jump_out_addr) / 4));

    append(JMP((a + 8) / 4)); // skip the next dword, which is an inline literal
    append(0xf0000000); // console putchar address
    append(LOAD(reg::R1, reg::PC, opsize::SIZE_32, -4)); // R1 = console putchar addr
    append(MOVIU(reg::R2, 'F'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(MOVIU(reg::R2, 'a'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(MOVIU(reg::R2, 'i'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(MOVIU(reg::R2, 'l'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(MOVIU(reg::R2, 'u'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(MOVIU(reg::R2, 'r'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(MOVIU(reg::R2, 'e'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(MOVIU(reg::R2, '\n'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(MOVIU(reg::R0, 0xBADBAD));
    append(HALT(0x0));
}

int main()
{
    test_memory mem;

    const int testaddr = 0x100;

    write_vectors(mem, testaddr / 4);
    write_memtest_program(mem, testaddr);

    fwrite(mem.memory, sizeof(mem.memory), 1, stdout);
}
