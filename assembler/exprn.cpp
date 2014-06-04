#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <utility>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

typedef unsigned int uint;

struct ExprBase;

typedef std::pair<boost::shared_ptr<ExprBase>, uint> ExprLine;
typedef std::map<std::string, ExprLine> labels_map;

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

bool ExprInt::eval(labels_map& labels, uint line, uint *value)
{
    *value = u;
    return true;
}

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

// when encountering .define:
// labels[identifier] = ExprLine(expr, curLine);
//
// when padding and storing label:
// labels[identifier] = ExprLine(ExprInt(address), curLine);
//
// when creating instruction:
// store expression
// then, later, when resolving instruction,
// bool success = expr->eval(labels, v);

int main()
{
    labels_map labels;

    ExprInt::sptr id5(new ExprInt(12));
    ExprIdent::sptr id(new ExprIdent("foo"));
    ExprIdent::sptr id2(new ExprIdent("fib"));
    ExprIdent::sptr id3(new ExprIdent("blarg"));
    ExprIdent::sptr id4(new ExprIdent("boo"));
    ExprIdent::sptr id6(new ExprIdent("feeb"));
    labels["feeb"] = ExprLine(id5, 0);
    labels["boo"] = ExprLine(id3, 0);
    labels["blarg"] = ExprLine(id4, 31415);
    labels["able"] = ExprLine(id3, 12345);
    uint v;
    bool r;
    
    r = id4->eval(labels, 4545, &v);
    if(!r)
        printf("id4 failed\n");
    else
        printf("id4 = %d\n", v);

    r = id2->eval(labels, 666, &v);
    if(!r)
        printf("id2 failed\n");
    else
        printf("id2 = %d\n", v);

    r = id6->eval(labels, 999, &v);
    if(!r)
        printf("failed\n");
    else
        printf("id6 = %d\n", v);

}
