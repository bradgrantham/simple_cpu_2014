#include "parsing.h"

bool ExprInt::eval(labels_map& labels, uint line, uint *value)
{
    *value = u;
    return true;
}

bool ExprIdent::eval(labels_map& labels, uint line, uint *value)
{
    if(visited) {
        std::cerr << "circular reference to identifier \"" << s << "\" at line " << line << " will evaluate to 0" << std::endl;
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

    std::cerr << "unresolved identifier \"" << s << "\" in expression at line " << line << " will evaluate to 0" << std::endl;
    *value = 0;
    visited = false;
    return false;
}

bool InstructionDirect::Store(OutputFile& file)
{
    uint instruction = simple_cpu_2014::format27(opcode, 0);
    file.Store32(address, instruction);
    return true;
}

bool StoreInstructions(OutputFile& file, std::vector<Instruction::sptr>& instrs)
{
    for(auto it = instrs.begin(); it != instrs.end(); it++) {
        bool result = (*it)->Store(file); 
        if(!result)
            return false;
    }
    return true;
}
