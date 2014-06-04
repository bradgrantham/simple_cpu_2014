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

bool ResolveExpression(const std::string& str, uint linenum, labels_map& labels, uint *value);

struct ExprBase
{
    typedef boost::shared_ptr<ExprBase> sptr;
    virtual bool eval(labels_map& labels, uint linenum, uint *value) = 0;
    virtual ~ExprBase() {}
};

struct ExprInt : public ExprBase
{
    typedef boost::shared_ptr<ExprInt> sptr;
    uint u;
    virtual bool eval(labels_map& labels, uint linenum, uint *value);
    ExprInt(uint u_) :
        u(u_)
        {}
    virtual ~ExprInt() {}
};

struct ExprIdent : public ExprBase
{
    typedef boost::shared_ptr<ExprIdent> sptr;
    std::string s;
    virtual bool eval(labels_map& labels, uint linenum, uint *value);
    ExprIdent(const std::string &s_) :
        s(s_),
        visited(false)
        {}
    virtual ~ExprIdent() {}
private:
    bool visited;
};

typedef std::vector<ExprBase::sptr> ExprList;

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
    void Store16(uint address, uint v)
    {
        printf("%08X: %04X\n", address, v);
    }
    void Store8(uint address, uint v)
    {
        printf("%08X: %02X\n", address, v);
    }
    void Finish()
    {
    }
};

struct Instruction
{
    typedef boost::shared_ptr<Instruction> sptr;
    uint address;
    uint linenum;
    uint opcode;
    virtual bool Store(labels_map& labels, OutputFile& file) = 0;
    Instruction(uint address_, uint linenum_, uint opcode_) :
        address(address_),
        linenum(linenum_),
        opcode(opcode_)
    {}
    virtual ~Instruction() {}
};

struct InstructionDirect : public Instruction
{
    typedef boost::shared_ptr<InstructionDirect> sptr;
    InstructionDirect(uint address_, uint linenum_, uint opcode_) :
        Instruction(address_, linenum_, opcode_)
        {}
    virtual bool Store(labels_map& labels, OutputFile& file);
    virtual ~InstructionDirect() {}
};

struct InstructionRX : public Instruction
{
    uint rx;
    typedef boost::shared_ptr<InstructionRX> sptr;
    InstructionRX(uint address_, uint linenum_, uint opcode_, uint rx_) :
        Instruction(address_, linenum_, opcode_),
        rx(rx_)
        {}
    virtual bool Store(labels_map& labels, OutputFile& file);
    virtual ~InstructionRX() {}
};

struct InstructionRXRY : public Instruction
{
    uint rx;
    uint ry;
    typedef boost::shared_ptr<InstructionRXRY> sptr;
    InstructionRXRY(uint address_, uint linenum_, uint opcode_, uint rx_, uint ry_) :
        Instruction(address_, linenum_, opcode_),
        rx(rx_),
        ry(ry_)
        {}
    virtual bool Store(labels_map& labels, OutputFile& file);
    virtual ~InstructionRXRY() {}
};

struct InstructionImm : public Instruction
{
    ExprBase::sptr imm;
    typedef boost::shared_ptr<InstructionImm> sptr;
    InstructionImm(uint address_, uint linenum_, uint opcode_, const ExprBase::sptr& imm_) :
        Instruction(address_, linenum_, opcode_),
        imm(imm_)
        {}
    virtual bool Store(labels_map& labels, OutputFile& file);
    virtual ~InstructionImm() {}
};

struct InstructionRXImmModified : public Instruction
{
    uint modifier;
    uint rx;
    ExprBase::sptr imm;
    typedef boost::shared_ptr<InstructionRXImmModified> sptr;
    InstructionRXImmModified(uint address_, uint linenum_, uint opcode_, uint modifier_, uint rx_, const ExprBase::sptr& imm_) :
        Instruction(address_, linenum_, opcode_),
        modifier(modifier_),
        rx(rx_),
        imm(imm_)
        {}
    virtual bool Store(labels_map& labels, OutputFile& file);
    virtual ~InstructionRXImmModified() {}
};

struct InstructionRXImm : public Instruction
{
    uint rx;
    ExprBase::sptr imm;
    typedef boost::shared_ptr<InstructionRXImm> sptr;
    InstructionRXImm(uint address_, uint linenum_, uint opcode_, uint rx_, const ExprBase::sptr& imm_) :
        Instruction(address_, linenum_, opcode_),
        rx(rx_),
        imm(imm_)
        {}
    virtual bool Store(labels_map& labels, OutputFile& file);
    virtual ~InstructionRXImm() {}
};

struct InstructionRXRYImmModified : public Instruction
{
    uint modifier;
    uint rx;
    uint ry;
    ExprBase::sptr imm;
    typedef boost::shared_ptr<InstructionRXRYImmModified> sptr;
    InstructionRXRYImmModified(uint address_, uint linenum_, uint opcode_, uint modifier_, uint rx_, uint ry_, const ExprBase::sptr& imm_) :
        Instruction(address_, linenum_, opcode_),
        modifier(modifier_),
        rx(rx_),
        ry(ry_),
        imm(imm_)
        {}
    virtual bool Store(labels_map& labels, OutputFile& file);
    virtual ~InstructionRXRYImmModified() {}
};


struct Store
{
    uint linenum;
    uint address;
    int size;
    ExprList exprs;
    Store(uint linenum_, uint address_, int size_, const ExprList& exprs_) :
        linenum(linenum_),
        address(address_),
        size(size_),
        exprs(exprs_)
    {
    }
};

bool StoreInstructions(labels_map& labels, OutputFile& file, std::vector<Instruction::sptr>& instrs);
bool StoreMemoryDirectives(labels_map& labels, OutputFile& file, std::vector<Store>& stores);

