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
    // XXX check size of item
    uint instruction = simple_cpu_2014::format27(opcode, u);
    file.Store32(address, instruction);
    return success;
}

bool InstructionRXImm::Store(labels_map& labels, OutputFile& file)
{
    unsigned int u;
    bool success = imm->eval(labels, linenum, &u);
    // XXX check size of item
    uint instruction = simple_cpu_2014::format24(opcode, rx, u);
    file.Store32(address, instruction);
    return success;
}

bool InstructionRXImmModified::Store(labels_map& labels, OutputFile& file)
{
    unsigned int u;
    bool success = imm->eval(labels, linenum, &u);
    // XXX check size of item
    uint instruction = simple_cpu_2014::format21(opcode, rx, modifier, u);
    file.Store32(address, instruction);
    return success;
}

bool InstructionRXRYImmModified::Store(labels_map& labels, OutputFile& file)
{
    unsigned int u;
    bool success = imm->eval(labels, linenum, &u);
    // XXX check size of item
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
