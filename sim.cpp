#include <cstdlib>
#include <cstdio>
#include "simple_cpu_2014.hpp"

const int memsize = 16 * 1024 * 1024;
const int registercount = 8;

using namespace simple_cpu_2014;

int32_t sign_extend(uint32_t v, int bits)
{
    bool negative = v & (1 << (bits - 1));
    return negative ? (v | (0xffffffff << bits)) : v;
}

struct state
{
    int32_t registers[registercount];
    bool lt, eq, gt, halted;
    int carry;
    uint8_t memory[memsize];

    uint32_t fetch32(uint32_t addr)
    {
        return
            (memory[addr + 0] << 0) | 
            (memory[addr + 1] << 8) | 
            (memory[addr + 2] << 16) | 
            (memory[addr + 3] << 24);
    }

    uint32_t fetch16(uint32_t addr)
    {
        return
            (memory[addr + 0] << 0) | 
            (memory[addr + 1] << 8);
    }

    uint32_t fetch8(uint32_t addr)
    {
        return memory[addr + 0];
    }

    void store32(uint32_t addr, uint32_t value)
    {
        memory[addr + 0] = (value >> 0) & 0xff;
        memory[addr + 1] = (value >> 8) & 0xff;
        memory[addr + 2] = (value >> 16) & 0xff;
        memory[addr + 3] = (value >> 24) & 0xff;
    }

    void store16(uint32_t addr, uint32_t value)
    {
        memory[addr + 0] = (value >> 0) & 0xff;
        memory[addr + 1] = (value >> 8) & 0xff;
    }

    void store8(uint32_t addr, uint32_t value)
    {
        memory[addr] = value;
    }

    state() { reset(); }
    void reset()
    {
        for(int i = 0; i < registercount; i++)
            registers[i] = 0 ;
        carry = 0;
        lt = false;
        eq = false;
        gt = false;
        halted = false;
        registers[reg::PC] = 0x0;
    }
};

struct instruction
{
    uint opcode;
    uint dst;
    uint src;
    uint size;
    uint data;
    instruction(uint32_t value);
};

typedef std::pair<bool, uint32_t> memory_changed;

typedef memory_changed (*instructionfunc)(state&, const instruction&);

struct opcode_info {
    int datasize;
    const char *name;
    instructionfunc func;
};

int datasize[] =
{
    [opcode::MOVIU] = 24,
    [opcode::ADDI] = 24,
    [opcode::SHIFT] = 24,
    [opcode::CMPIU] = 24,
    [opcode::ADDIU] = 24,

    [opcode::STORE] = 18,
    [opcode::LOAD] = 18,
    [opcode::MOV] = 18,
    [opcode::PUSH] = 24,
    [opcode::POP] = 24,

    [opcode::AND] = 18,
    [opcode::OR] = 18,
    [opcode::XOR] = 18,
    [opcode::NOT] = 18,
    [opcode::ADD] = 18,
    [opcode::ADC] = 18,
    [opcode::SUB] = 18,
    [opcode::MULT] = 18,
    [opcode::DIV] = 18,
    [opcode::CMP] = 18,
    [opcode::XCHG] = 18,

    [opcode::JNE] = 27,
    [opcode::JL] = 27,
    [opcode::JSR] = 27,
    [opcode::JMP] = 27,
    [opcode::JR] = 24,
    [opcode::RSR] = 24,

    [opcode::SYS] = 6, // special case
    [opcode::HALT] = 27,
};

instruction::instruction(uint32_t value)
{
    opcode = (value >> 27);
    dst = (value >> 24) & 0x7;
    src = (value >> 21) & 0x7;
    size = (value >> 18) & 0x7;
    data = value & maskbits(datasize[opcode]);
}

memory_changed moviu(state& s, const instruction& instr)
{
    s.registers[instr.dst] = instr.data;
    s.registers[reg::PC] += 4;
    return memory_changed(false, 0);
}

memory_changed addi(state& s, const instruction& instr)
{
    s.registers[instr.dst] += sign_extend(instr.data, datasize[instr.opcode]);
    s.registers[reg::PC] += 4;
    return memory_changed(false, 0);
}

memory_changed addiu(state& s, const instruction& instr)
{
    s.registers[instr.dst] += instr.data;
    s.registers[reg::PC] += 4;
    return memory_changed(false, 0);
}

memory_changed shift(state& s, const instruction& instr)
{
    if(instr.data & 0x20)
        s.registers[instr.dst] >>= (instr.data & 0x1f);
    else
        s.registers[instr.dst] <<= (instr.data & 0x1f);

    s.registers[reg::PC] += 4;
    return memory_changed(false, 0);
}

memory_changed cmpiu(state& s, const instruction& instr)
{
    s.eq = (((uint32_t)s.registers[instr.dst] & 0xffffff) == instr.data);
    s.lt = (((uint32_t)s.registers[instr.dst] & 0xffffff) < instr.data);
    s.gt = (((uint32_t)s.registers[instr.dst] & 0xffffff) > instr.data);

    s.registers[reg::PC] += 4;
    return memory_changed(false, 0);
}

memory_changed store(state& s, const instruction& instr)
{
    uint32_t addr = s.registers[instr.dst] + sign_extend(instr.data, 24);
    switch(instr.size) {
        case opsize::SIZE_8: s.store8(addr, s.registers[instr.src]); break;
        case opsize::SIZE_16: s.store16(addr, s.registers[instr.src]); break;
        case opsize::SIZE_32: s.store32(addr, s.registers[instr.src]); break;
    }
    s.registers[reg::PC] += 4;
    return memory_changed(true, addr);
}

memory_changed load(state& s, const instruction& instr)
{
    uint32_t addr = s.registers[instr.src] + sign_extend(instr.data, 24);
    switch(instr.size) {
        case opsize::SIZE_8: s.registers[instr.dst] = s.fetch8(addr); break;
        case opsize::SIZE_16: s.registers[instr.dst] = s.fetch16(addr); break;
        case opsize::SIZE_32: s.registers[instr.dst] = s.fetch32(addr); break;
    }
    s.registers[reg::PC] += 4;
    return memory_changed(false, 0);
}

memory_changed mov(state& s, const instruction& instr)
{
    s.registers[instr.dst] = s.registers[instr.src];
    s.registers[reg::PC] += 4;
    return memory_changed(false, 0);
}

memory_changed push(state& s, const instruction& instr)
{
    s.registers[reg::SP] -= 4;
    s.store32(s.registers[reg::SP], s.registers[instr.dst]); // dst is first reg
    s.registers[reg::PC] += 4;
    return memory_changed(true, s.registers[reg::SP]);
}

memory_changed pop(state& s, const instruction& instr)
{
    s.registers[instr.dst] = s.fetch32(s.registers[reg::SP]);
    s.registers[reg::SP] += 4;
    s.registers[reg::PC] += 4;
    return memory_changed(false, 0);
}

memory_changed op_and(state& s, const instruction& instr)
{
    s.registers[instr.dst] &= s.registers[instr.src];
    s.registers[reg::PC] += 4;
    return memory_changed(false, 0);
}

memory_changed op_or(state& s, const instruction& instr)
{
    s.registers[instr.dst] |= s.registers[instr.src];
    s.registers[reg::PC] += 4;
    return memory_changed(false, 0);
}

memory_changed op_xor(state& s, const instruction& instr)
{
    s.registers[instr.dst] ^= s.registers[instr.src];
    s.registers[reg::PC] += 4;
    return memory_changed(false, 0);
}

memory_changed op_not(state& s, const instruction& instr)
{
    s.registers[instr.dst] = ~s.registers[instr.src];
    s.registers[reg::PC] += 4;
    return memory_changed(false, 0);
}

memory_changed add(state& s, const instruction& instr)
{
    s.registers[instr.dst] += s.registers[instr.src];
    s.registers[reg::PC] += 4;
    return memory_changed(false, 0);
}

memory_changed adc(state& s, const instruction& instr)
{
    s.registers[instr.dst] += s.registers[instr.src] + s.carry;
    s.registers[reg::PC] += 4;
    return memory_changed(false, 0);
}

memory_changed sub(state& s, const instruction& instr)
{
    s.registers[instr.dst] -= s.registers[instr.src];
    s.registers[reg::PC] += 4;
    return memory_changed(false, 0);
}

memory_changed mult(state& s, const instruction& instr)
{
    long long v = s.registers[instr.dst] * s.registers[instr.src];
    s.registers[instr.dst] = v >> 32;
    s.registers[instr.src] = v & 0xffffffff;
    s.registers[reg::PC] += 4;
    return memory_changed(false, 0);
}

memory_changed div(state& s, const instruction& instr)
{
    int32_t d = s.registers[instr.dst] / s.registers[instr.src];
    int32_t m = s.registers[instr.dst] % s.registers[instr.src];
    s.registers[instr.dst] = d;
    s.registers[instr.src] = m;
    s.registers[reg::PC] += 4;
    return memory_changed(false, 0);
}

memory_changed cmp(state& s, const instruction& instr)
{
    s.eq = (s.registers[instr.dst] == s.registers[instr.src]);
    s.lt = (s.registers[instr.dst] < s.registers[instr.src]);
    s.gt = (s.registers[instr.dst] > s.registers[instr.src]);
    s.registers[reg::PC] += 4;
    return memory_changed(false, 0);
}

memory_changed xchg(state& s, const instruction& instr)
{
    int32_t t = s.registers[instr.dst];
    s.registers[instr.dst] = s.registers[instr.src];
    s.registers[instr.src] = t;
    s.registers[reg::PC] += 4;
    return memory_changed(false, 0);
}

memory_changed jne(state& s, const instruction& instr)
{
    if(s.eq)
        s.registers[reg::PC] += 4;
    else
        s.registers[reg::PC] += sign_extend(instr.data, 27) << 2;
    return memory_changed(false, 0);
}

memory_changed jl(state& s, const instruction& instr)
{
    if(s.lt)
        s.registers[reg::PC] += sign_extend(instr.data, 27) << 2; // XXX proposed
    else
        s.registers[reg::PC] += 4;
    return memory_changed(false, 0);
}

memory_changed jsr(state& s, const instruction& instr)
{
    // Rx <= pc, pc <= pc + (sdata24 << 2))
    s.registers[instr.dst] = s.registers[reg::PC] + 4; // XXX proposed
    s.registers[reg::PC] += sign_extend(instr.data, 27) << 2;
    return memory_changed(false, 0);
}

memory_changed rsr(state& s, const instruction& instr)
{
    s.registers[reg::PC] = s.registers[instr.dst];
    return memory_changed(false, 0);
}

memory_changed jmp(state& s, const instruction& instr)
{
    s.registers[reg::PC] = sign_extend(instr.data, 27) << 2;
    return memory_changed(false, 0);
}

memory_changed jr(state& s, const instruction& instr)
{
    s.registers[reg::PC] = s.registers[instr.dst] + (sign_extend(instr.data, 24) << 2);
    return memory_changed(false, 0);
}

memory_changed sys(state& s, const instruction& instr)
{
    s.registers[reg::SP] -= 4;
    s.store32(s.registers[reg::SP], s.registers[reg::PC]); // dst is first reg
    s.registers[reg::PC] = instr.data << 2;
    return memory_changed(true, s.registers[reg::SP]);
}

memory_changed halt(state& s, const instruction& instr)
{
    s.halted = true;
    return memory_changed(false, 0);
}

const char* opcode_names[] =
{
    [opcode::MOVIU] = "moviu",
    [opcode::ADDI] = "addi",
    [opcode::SHIFT] = "shift",
    [opcode::CMPIU] = "cmpiu",
    [opcode::ADDIU] = "addiu",

    [opcode::STORE] = "store",
    [opcode::LOAD] = "load",
    [opcode::MOV] = "mov",
    [opcode::PUSH] = "push",
    [opcode::POP] = "pop",

    [opcode::AND] = "and",
    [opcode::OR] = "or",
    [opcode::XOR] = "xor",
    [opcode::NOT] = "not",
    [opcode::ADD] = "add",
    [opcode::ADC] = "adc",
    [opcode::SUB] = "sub",
    [opcode::MULT] = "mult",
    [opcode::DIV] = "div",
    [opcode::CMP] = "cmp",
    [opcode::XCHG] = "xchg",

    [opcode::JNE] = "jne",
    [opcode::JL] = "jl",
    [opcode::JSR] = "jsr",
    [opcode::JMP] = "jmp",
    [opcode::JR] = "jr",
    [opcode::RSR] = "rsr",

    [opcode::SYS] = "sys", // special case
    [opcode::HALT] = "halt",
};

instructionfunc funcs[] =
{
    [opcode::MOVIU] = moviu,
    [opcode::ADDI] = addi,
    [opcode::SHIFT] = shift,
    [opcode::CMPIU] = cmpiu,
    [opcode::ADDIU] = addiu,

    [opcode::STORE] = store,
    [opcode::LOAD] = load,
    [opcode::MOV] = mov,
    [opcode::PUSH] = push,
    [opcode::POP] = pop,

    [opcode::AND] = op_and,
    [opcode::OR] = op_or,
    [opcode::XOR] = op_xor,
    [opcode::NOT] = op_not,
    [opcode::ADD] = add,
    [opcode::ADC] = adc,
    [opcode::SUB] = sub,
    [opcode::MULT] = mult,
    [opcode::DIV] = div,
    [opcode::CMP] = cmp,
    [opcode::XCHG] = xchg,

    [opcode::JNE] = jne,
    [opcode::JL] = jl,
    [opcode::JSR] = jsr,
    [opcode::JMP] = jmp,
    [opcode::JR] = jr,
    [opcode::RSR] = rsr,

    [opcode::SYS] = sys, // special case
    [opcode::HALT] = halt,
};



state s;

int main(int argc, char **argv)
{
    uint32_t addr = 0;
    uint32_t d;
    while(fread(&d, 4, 1, stdin) == 1) {
        s.store32(addr, d);
        addr += 4;
    }

    while(!s.halted) {
        instruction instr(s.fetch32(s.registers[reg::PC]));

        printf("decoded %s", opcode_names[instr.opcode]);
        if(datasize[instr.opcode] == 18) {
            printf(", dst = %d, src = %d, size = %d, data = 0x%X\n", instr.dst, instr.src, instr.size, instr.data);
        } else if(datasize[instr.opcode] == 24) {
            printf(", dst = %d, src = %d, data = 0x%X\n", instr.dst, instr.src, instr.data);
        } else /* if(datasize[instr.opcode] == 27) or 6 */ {
            printf(", dst = %d, data = 0x%X\n", instr.dst, instr.data);
        }

        memory_changed change = funcs[instr.opcode](s, instr);

        printf("R0:%08X R1:%08X R2:%08X R3:%08X\nR4:%08X R5:%08X SP:%08X PC:%08X\n", 
            s.registers[0], s.registers[1], s.registers[2], s.registers[3],
            s.registers[4], s.registers[5], s.registers[6], s.registers[7]);
        if(change.first) {
            printf("changed %08x : %08X\n", change.second, s.fetch32(change.second));
        }
    };
}
