#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <utility>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

typedef unsigned int uint;

struct ExprBase;

typedef std::map<std::string, uint> labels_resolved_map;
typedef std::pair<boost::shared_ptr<ExprBase>, uint> expr_linenum;
typedef std::map<std::string, expr_linenum> labels_unresolved_map;

bool ResolveExpression(labels_resolved_map& resolved, labels_unresolved_map& unresolved, const std::string& str, boost::shared_ptr<ExprBase> expr, uint line, uint *value);

struct ExprBase
{
    typedef boost::shared_ptr<ExprBase> sptr;
    virtual bool eval(labels_resolved_map& resolved, labels_unresolved_map& unresolved, uint line, uint *value) = 0;
    virtual ~ExprBase() {}
};

struct ExprInt : public ExprBase
{
    typedef boost::shared_ptr<ExprInt> sptr;
    uint u;
    virtual bool eval(labels_resolved_map& resolved, labels_unresolved_map& unresolved, uint line, uint *value);
    ExprInt(uint u_) :
        u(u_)
        {}
    virtual ~ExprInt() {}
};

bool ExprInt::eval(labels_resolved_map& resolved, labels_unresolved_map& unresolved, uint line, uint *value)
{
    *value = u;
    return true;
}

struct ExprIdent : public ExprBase
{
    typedef boost::shared_ptr<ExprIdent> sptr;
    std::string s;
    virtual bool eval(labels_resolved_map& resolved, labels_unresolved_map& unresolved, uint line, uint *value);
    ExprIdent(const std::string &s_) :
        s(s_),
        visited(false)
        {}
    virtual ~ExprIdent() {}
private:
    bool visited;
};

bool ExprIdent::eval(labels_resolved_map& resolved, labels_unresolved_map& unresolved, uint line, uint *value)
{
    if(visited) {
        std::cerr << "circular reference to identifier \"" << s << "\" at line " << line << " will evaluate to 0" << std::endl;
        *value = 0;
        return false;
    }

    visited = true;
    auto res = resolved.find(s);
    if(res != resolved.end()) {
        *value = res->second;
        visited = false;
        return true;
    }

    auto unres = unresolved.find(s);
    if(unres != unresolved.end()) {
        bool result = ResolveExpression(resolved, unresolved, s, unres->second.first, unres->second.second, value);
        visited = false;
        return result;
    }

    *value = 0;
    visited = false;
    return false;
}

labels_resolved_map labels_resolved;
labels_unresolved_map labels_unresolved;

bool ResolveExpression(labels_resolved_map& resolved, labels_unresolved_map& unresolved, const std::string& str, ExprBase::sptr expr, uint line, uint *value)
{
    bool result = expr->eval(resolved, unresolved, line, value);
    if(!result) {
        std::cerr << "unresolved identifier \"" << str << "\" in expression at line " << line << " will evaluate to 0" << std::endl;
        *value = 0;
    }
    resolved[str] = *value;
    return result;
}

bool ResolveUnresolved(labels_resolved_map& resolved, labels_unresolved_map& unresolved)
{
    bool success = true;
    labels_unresolved_map::iterator unres;
    while((unres = unresolved.begin()) != unresolved.end()) {
        std::string s = unres->first;
        expr_linenum el = unres->second;
        unresolved.erase(unres);
        uint value;
        bool result = ResolveExpression(resolved, unresolved, s, el.first, el.second, &value);
        resolved[s] = value;
        success = success && result;
    }
    return success;
}

// when encountering .define:
// labels_unresolved[identifier] = expr_linenum(expr, 0);
//
// when padding and storing label:
// labels_resolved[identifier] = address
//
// At end of file, ResolveUnresolved(resolved, unresolved); 
//
// when creating instruction:
// store expression
// then, later, when resolving instruction,
// expr->eval(resolved, unresolved

int main()
{
    labels_resolved["foo"] = 5;
    labels_resolved["bar"] = 5;
    ExprInt::sptr id5(new ExprInt(12));
    ExprIdent::sptr id(new ExprIdent("foo"));
    ExprIdent::sptr id2(new ExprIdent("fib"));
    ExprIdent::sptr id3(new ExprIdent("blarg"));
    ExprIdent::sptr id4(new ExprIdent("boo"));
    labels_unresolved["feeb"] = expr_linenum(id5, 0);
    labels_unresolved["boo"] = expr_linenum(id3, 0);
    labels_unresolved["blarg"] = expr_linenum(id4, 31415);
    labels_unresolved["able"] = expr_linenum(id3, 12345);
    uint v;
    // bool r = ResolveExpression(labels_resolved, labels_unresolved, "baz", id, 0, &v);
    bool r = ResolveUnresolved(labels_resolved, labels_unresolved);
    if(!r)
        printf("failed\n");
    for(auto it = labels_resolved.begin(); it != labels_resolved.end(); it++) {
        printf("labels_resolved[%s] = %d\n", it->first.c_str(), it->second);
    }
}
