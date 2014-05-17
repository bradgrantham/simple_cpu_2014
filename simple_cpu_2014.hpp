#include <boost/cstdint.hpp> // boost 1.55, -I/opt/include/local

namespace simple_cpu_2014 {

static uint32_t maskbits(int count)
{
    return (1 << count) - 1;
}

static inline uint32_t format16(uint32_t op, uint32_t dst, uint32_t src, uint32_t size, uint32_t data16)
{
    return
        (op << 27) | 
        (dst << 24) | 
        (src << 21) | 
        (size << 18) | 
        (data16 & maskbits(16)) ;

}

template <uint32_t OP>
uint32_t format16_(uint32_t dst, uint32_t src, uint32_t size, uint32_t data16)
{
    return format16(OP, dst, src, size, data16);
}


static inline uint32_t format18(uint32_t op, uint32_t dst, uint32_t src, uint32_t size, uint32_t data18)
{
    return
        (op << 27) | 
        (dst << 24) | 
        (src << 21) | 
        (size << 18) | 
        (data18 & maskbits(18)) ;

}

template <uint32_t OP>
uint32_t format18_(uint32_t dst, uint32_t src, uint32_t size, uint32_t data18)
{
    return format18(OP, dst, src, size, data18);
}


static inline uint32_t format24(uint32_t op, uint32_t dst, uint32_t data24)
{
    return
        (op << 27) | 
        (dst << 24) | 
        (data24 & maskbits(24)) ;

}
 
template <uint32_t OP>
uint32_t format24_(uint32_t dst, uint32_t data24)
{
    return format24(OP, dst, data24);
}

static inline uint32_t format27(uint32_t op, uint32_t data27)
{
    return
        (op << 27) | 
        (data27 & maskbits(27)) ;

}

template <uint32_t OP>
uint32_t format27_(uint32_t data27)
{
    return format27(OP, data27);
}

typedef unsigned int uint;

namespace opcode {
    const uint AND = 0x00;
    const uint OR = 0x01;
    const uint XOR = 0x02;
    const uint NOT = 0x03;
    const uint ADD = 0x04;
    const uint ADC = 0x05;
    const uint SUB = 0x06;
    const uint MULT = 0x07;
    const uint DIV = 0x08;
    const uint CMP = 0x09;
    const uint XCHG = 0x0a;

    const uint MOV = 0x0b;
    const uint LOAD = 0x0c;
    const uint STORE = 0x0d;
    const uint PUSH = 0x0e;
    const uint POP = 0x0f;

    const uint MOVIU = 0x10;
    const uint ADDI = 0x11;
    const uint ADDIU = 0x12;
    const uint CMPIU = 0x13;
    const uint SHIFT = 0x14;

    const uint JL = 0x15;
    const uint JNE = 0x16;
    const uint JR = 0x17;
    const uint JSR = 0x18;
    const uint RSR = 0x19;
    const uint JMP = 0x1a;

    const uint SYS = 0x1b;
    const uint UNUSED_1c = 0x1c;
    const uint UNUSED_1d = 0x1d;
    const uint UNUSED_1e = 0x1e;
    const uint HALT = 0x1f;
};

namespace opsize {
    const uint SIZE_8 = 0;
    const uint SIZE_16 = 1;
    const uint SIZE_32 = 2;
};

namespace reg {
    const uint R0 = 0;
    const uint R1 = 1;
    const uint R2 = 2;
    const uint R3 = 3;
    const uint R4 = 4;
    const uint R5 = 5;
    const uint R6 = 6;
    const uint SP = 6;
    const uint R7 = 7;
    const uint PC = 7;
};

auto const MOVIU = format24_<opcode::MOVIU>;
auto const ADDI = format24_<opcode::ADDI>;
auto const SHIFT = format24_<opcode::SHIFT>;
auto const CMPIU = format24_<opcode::CMPIU>;
auto const ADDIU = format24_<opcode::ADDIU>;

auto const STORE = format18_<opcode::STORE>;
auto const LOAD = format18_<opcode::LOAD>;
auto const MOV = format18_<opcode::MOV>;
auto const PUSH = format24_<opcode::PUSH>;
auto const POP = format24_<opcode::POP>;

auto const AND = format18_<opcode::AND>;
auto const OR = format18_<opcode::OR>;
auto const XOR = format18_<opcode::XOR>;
auto const NOT = format18_<opcode::NOT>;
auto const ADD = format18_<opcode::ADD>;
auto const ADC = format18_<opcode::ADC>;
auto const SUB = format18_<opcode::SUB>;
auto const MULT = format18_<opcode::MULT>;
auto const DIV = format18_<opcode::DIV>;
auto const CMP = format18_<opcode::CMP>;
auto const XCHG = format18_<opcode::XCHG>;

auto const JNE = format27_<opcode::JNE>;
auto const JL = format27_<opcode::JL>;
auto const JSR = format27_<opcode::JSR>;
auto const JMP = format27_<opcode::JMP>;
auto const JR = format24_<opcode::JR>;
auto const RSR = format24_<opcode::RSR>;

auto const SYS = format27_<opcode::SYS>; // only LS 6 bits are used
auto const HALT = format27_<opcode::HALT>;

};

