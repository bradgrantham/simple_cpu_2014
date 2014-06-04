#include <map>
#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "simple_cpu_2014.hpp"

typedef unsigned int uint;

struct ExprBase;

typedef std::pair<boost::shared_ptr<ExprBase>, uint> LineNumberExpr;
typedef std::map<std::string, LineNumberExpr> labels_map;

bool ResolveExpression(const std::string& str, uint line, labels_map& labels, uint *value);

struct ExprBase
{
    typedef boost::shared_ptr<ExprBase> sptr;
    virtual bool eval(labels_map& labels, uint line, uint *value) = 0;
    virtual ~ExprBase() {}
};

struct ExprInt : public ExprBase
{
    typedef boost::shared_ptr<ExprInt> sptr;
    uint u;
    virtual bool eval(labels_map& labels, uint line, uint *value);
    ExprInt(uint u_) :
        u(u_)
        {}
    virtual ~ExprInt() {}
};

struct ExprIdent : public ExprBase
{
    typedef boost::shared_ptr<ExprIdent> sptr;
    std::string s;
    virtual bool eval(labels_map& labels, uint line, uint *value);
    ExprIdent(const std::string &s_) :
        s(s_),
        visited(false)
        {}
    virtual ~ExprIdent() {}
private:
    bool visited;
};

typedef std::vector<ExprBase*> ExprList;

// labels_map labels;
//
// when encountering .define:
// labels[identifier] = LineNumberExpr(expr, curLine);
//
// when padding and storing label:
// labels[identifier] = LineNumberExpr(ExprInt(address), curLine);
//
// when creating instruction:
// store expression
// then, later, when resolving instruction,
// bool success = expr->eval(labels, v);

struct OutputFile
{
    void Store32(uint address, uint v)
    {
        printf("%08X: %08X\n", address, v);
    }
};

struct Instruction
{
    typedef boost::shared_ptr<Instruction> sptr;
    uint address;
    uint line;
    uint opcode;
    virtual bool Store(OutputFile& file) = 0;
    Instruction(uint address_, uint line_, uint opcode_) :
        address(address_),
        line(line_),
        opcode(opcode_)
    {}
    virtual ~Instruction() {}
};

struct InstructionDirect : public Instruction
{
    typedef boost::shared_ptr<InstructionDirect> sptr;
    InstructionDirect(uint address_, uint line_, uint opcode_) :
        Instruction(address_, line_, opcode_)
        {}
    virtual bool Store(OutputFile& file);
    virtual ~InstructionDirect() {}
};


bool StoreInstructions(OutputFile& file, std::vector<Instruction::sptr>& instrs);

