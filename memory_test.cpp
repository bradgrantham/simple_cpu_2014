#include <cassert>
#include <cstdio>
#include <cstring>
#include "simple_cpu_2014.hpp"

struct memory {
    virtual void w(uint32_t byteaddr, uint32_t data) = 0;
};

void write_vectors(memory& m, uint32_t reset)
{
    using namespace simple_cpu_2014;

    m.w(0, JMP(reset));
    // XXX interrupt vectors, also
}

void write_memtest_program(memory& m, uint32_t base)
{
    using namespace simple_cpu_2014;

    uint32_t a = base;

    auto store = [&] (uint32_t data) {
        m.w(a, data);
        a += 4;
    };

    // Program starts at 0x000000h
    // store(format24(opcode::MOVIU, reg::SP, 0)); // Stack starts at 0x800000
    store(MOVIU(reg::SP, 0)); // Stack starts at 0x800000
    store(ADDI(reg::SP, 0x800000));

    store(MOVIU(reg::R0, 0x000000)); // p test start a = 0x000100
    store(ADDI(reg::R0, 0x000100));
    store(MOVIU(reg::R1, 0x000000)); // data
    store(MOVIU(reg::R3, 0x000100)); // test “shift” (R3 should be 0x2000000 after next instr)
    store(SHIFT(reg::R3, 0x000001));

    uint32_t loop = a;
    store(STORE(reg::R0, reg::R1, opsize::SIZE_16, 0x000000));
    store(LOAD(reg::R2, reg::R0, opsize::SIZE_16, 0x000000));
    store(CMP(reg::R2, reg::R1, opsize::SIZE_32, 0x000000));
    store(JNE(0xc0baad - 0x09));
    store(ADDI(reg::R0, 2)); // +2 for 16-bit word a, XXX signed operand
    store(ADDI(reg::R1, 2)); // XXX signed operand
    store(CMPIU(reg::R0, 0x800000));
    int32_t delta = loop - a;
    store(JL(delta)); // XXX signed operand
    store(MOVIU(reg::R3, 0x1234)); // data
    store(ADDI(reg::R3, 0x5678)); // data
    store(PUSH(reg::R3, 0)); // data
    store(POP(reg::R4, 0)); // data
    store(STORE(reg::R5, reg::R3, opsize::SIZE_16, 0x000000));
    store(LOAD(reg::R6, reg::R5, opsize::SIZE_16, 0x000000));
}

struct test_memory : public memory {
    static const int memdwords = 1024;
    uint32_t memory[memdwords];

    test_memory()
    {
        memset(memory, 0, sizeof(memory));
    }

    virtual void w(uint32_t byteaddr, uint32_t data)
    {
        // XXX only handles align-4
        if(byteaddr / 4 < memdwords)
            memory[byteaddr / 4] = data;
    }
};


int main()
{
    test_memory mem;

    const int testaddr = 0x200;

    write_vectors(mem, testaddr);
    write_memtest_program(mem, testaddr);

    fwrite(mem.memory, sizeof(mem.memory), 1, stdout);
}
