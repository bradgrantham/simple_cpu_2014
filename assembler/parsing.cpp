#include <iostream>
#include "parsing.h"

bool ExprInt::eval(labels_map& labels, uint linenum, uint *value)
{
    *value = u;
    return true;
}

bool ExprIdent::eval(labels_map& labels, uint linenum, uint *value)
{
    if(visited) {
        std::cerr << "circular reference to identifier \"" << s << "\" at line " << linenum << " will evaluate to 0" << std::endl;
        *value = 0;
        return false;
    }

    visited = true;

    auto unres = labels.find(s);
    if(unres != labels.end()) {
        ExprBase::sptr expr(unres->second.first);
        uint linenum = unres->second.second;
        bool result = expr->eval(labels, linenum, value);
        visited = false;
        return result;
    }

    std::cerr << "unresolved identifier \"" << s << "\" in expression at line " << linenum << " will evaluate to 0" << std::endl;
    *value = 0;
    visited = false;
    return false;
}

uint ImmediateOperandInfo::Encode(uint v, int line, uint address)
{
    uint mask = (1 << size) - 1;

    if(relative)
        v = v - address;

    uint expected = (signd && (v & 0x80000000)) ? (0xffffffff & ~mask) : 0;

    if(v & ((1 << shift) - 1)) {
        std::cerr << shift << " LSBs in immediate operand at line " << line << " were not zero but were lost" << std::endl;
    }

    if(!signd) {

        v = v >> shift;

    } else {

        v = int(v) >> shift;

    }

    if((v & ~mask) != expected) {
        std::cerr << "immediate operand at line " << line << " exceeded data field size of " << size << " bits (after right shift by " << shift << ") and was truncated" << std::endl;
    }

    return v & mask;
}

#pragma clang diagnostic ignored "-Wgnu-designator"

ImmediateOperandInfo instr_infos[] = {
    [simple_cpu_2014::opcode::AND] {0, false, 0, false},
    [simple_cpu_2014::opcode::OR] {0, false, 0, false},
    [simple_cpu_2014::opcode::XOR] {0, false, 0, false},
    [simple_cpu_2014::opcode::NOT] {0, false, 0, false},
    [simple_cpu_2014::opcode::ADD] {0, false, 0, false},
    [simple_cpu_2014::opcode::ADC] {0, false, 0, false},
    [simple_cpu_2014::opcode::SUB] {0, false, 0, false},
    [simple_cpu_2014::opcode::MULT] {0, false, 0, false},
    [simple_cpu_2014::opcode::DIV] {0, false, 0, false},
    [simple_cpu_2014::opcode::CMP] {0, false, 0, false},
    [simple_cpu_2014::opcode::XCHG] {0, false, 0, false},
    [simple_cpu_2014::opcode::MOV] {0, false, 0, false},

    [simple_cpu_2014::opcode::LOAD] {0, true, 18, false },
    [simple_cpu_2014::opcode::STORE] {0, true, 18, false },

    [simple_cpu_2014::opcode::PUSH] {0, false, 0, false},
    [simple_cpu_2014::opcode::POP] {0, false, 0, false},

    [simple_cpu_2014::opcode::MOVIU] {0, false, 16, false },
    [simple_cpu_2014::opcode::ADDI] {0, true, 24, false },
    [simple_cpu_2014::opcode::ADDIU] {0, false, 24, false },
    [simple_cpu_2014::opcode::CMPIU] {0, false, 24, false },
    [simple_cpu_2014::opcode::SHIFT] {0, false, 6, false },

    [simple_cpu_2014::opcode::JL] {2, true, 27, true },
    [simple_cpu_2014::opcode::JNE] {2, true, 27, true },

    [simple_cpu_2014::opcode::JR] {2, true, 24, false },
    [simple_cpu_2014::opcode::JSR] {2, true, 24, true },

    [simple_cpu_2014::opcode::RSR] {0, false, 0, false},

    [simple_cpu_2014::opcode::JMP] {2, false, 27, false },

    [simple_cpu_2014::opcode::SYS] {0, false, 6, false }, // special case - provide vector # to SYS

    [simple_cpu_2014::opcode::SWAPCC] {0, false, 0, false},
    [simple_cpu_2014::opcode::UNUSED_1d] {0, false, 0, false},
    [simple_cpu_2014::opcode::UNUSED_1e] {0, false, 0, false},
    [simple_cpu_2014::opcode::HALT] {0, false, 0, false},
};


bool InstructionDirect::Store(labels_map& labels, OutputFile& file)
{
    uint instruction = simple_cpu_2014::format27(opcode, 0);
    file.Store32(address, instruction);
    return true;
}

bool InstructionRXRY::Store(labels_map& labels, OutputFile& file)
{
    uint instruction = simple_cpu_2014::format18(opcode, rx, ry, 0, 0);
    file.Store32(address, instruction);
    return true;
}

bool InstructionRX::Store(labels_map& labels, OutputFile& file)
{
    uint instruction = simple_cpu_2014::format24(opcode, rx, 0);
    file.Store32(address, instruction);
    return true;
}

bool InstructionImm::Store(labels_map& labels, OutputFile& file)
{
    unsigned int u;
    bool success = imm->eval(labels, linenum, &u);

    u = instr_infos[opcode].Encode(u, linenum, address);

    uint instruction = simple_cpu_2014::format27(opcode, u);
    file.Store32(address, instruction);
    return success;
}

bool InstructionRXImm::Store(labels_map& labels, OutputFile& file)
{
    unsigned int u;
    bool success = imm->eval(labels, linenum, &u);

    u = instr_infos[opcode].Encode(u, linenum, address);

    uint instruction = simple_cpu_2014::format24(opcode, rx, u);
    file.Store32(address, instruction);
    return success;
}

bool InstructionRXImmModified::Store(labels_map& labels, OutputFile& file)
{
    unsigned int u;
    bool success = imm->eval(labels, linenum, &u);

    u = instr_infos[opcode].Encode(u, linenum, address);

    uint instruction = simple_cpu_2014::format21(opcode, rx, modifier, u);
    file.Store32(address, instruction);
    return success;
}

bool InstructionRXRYImmModified::Store(labels_map& labels, OutputFile& file)
{
    unsigned int u;
    bool success = imm->eval(labels, linenum, &u);

    u = instr_infos[opcode].Encode(u, linenum, address);

    uint instruction = simple_cpu_2014::format18(opcode, rx, ry, modifier, u);
    file.Store32(address, instruction);
    return success;
}

bool StoreInstructions(labels_map& labels, OutputFile& file, std::vector<Instruction::sptr>& instrs)
{
    for(auto it = instrs.begin(); it != instrs.end(); it++) {
        bool result = (*it)->Store(labels, file); 
        if(!result)
            return false;
    }
    return true;
}

bool StoreMemoryDirectives(labels_map& labels, OutputFile& file, std::vector<Store>& stores)
{
    bool success = true;
    for(auto it = stores.begin(); it != stores.end(); it++) {
        Store& store = *it;
        unsigned int address = store.address;
        for(auto m = store.exprs.begin(); m != store.exprs.end(); m++) {
            unsigned int u;
            bool result = (*m)->eval(labels, store.linenum, &u);
            // XXX check size of item
            if(store.size == 1)
                file.Store8(address, u);
            else if(store.size == 2)
                file.Store16(address, u);
            else if(store.size == 4)
                file.Store32(address, u);
            address += store.size;
            success = success && result;
        }
    }
    return success;
}
