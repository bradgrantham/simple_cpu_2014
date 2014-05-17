#include <cassert>
#include <cstdio>
#include <cstring>
#include "simple_cpu_2014.hpp"
#include "util.hpp"


void write_hello_program(memory& m, uint32_t base)
{
    using namespace simple_cpu_2014;

    uint32_t a = base;

    auto append = [&] (uint32_t data) {
        m.w(a, data);
        a += 4;
    };

    append(JMP((a + 8) / 4)); // skip the next dword, which is an inline literal
    append(0xf0000000); // console putchar address
    append(LOAD(reg::R1, reg::PC, opsize::SIZE_32, -4)); // R1 = console putchar addr
    append(MOVIU(reg::R2, 'H'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(MOVIU(reg::R2, 'e'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(MOVIU(reg::R2, 'l'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(MOVIU(reg::R2, 'l'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(MOVIU(reg::R2, 'o'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(MOVIU(reg::R2, '\n'));
    append(STORE(reg::R1, reg::R2, opsize::SIZE_8, 0x000000));
    append(HALT(0x0));
}

int main()
{
    test_memory mem;

    const int testaddr = 0x100;

    write_vectors(mem, testaddr / 4);
    write_hello_program(mem, testaddr);

    fwrite(mem.memory, sizeof(mem.memory), 1, stdout);
}
