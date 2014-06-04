#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <utility>
#include "parsing.h"

int main()
{
    labels_map labels;

    ExprInt::sptr id5(new ExprInt(12));
    ExprIdent::sptr id(new ExprIdent("foo"));
    ExprIdent::sptr id2(new ExprIdent("fib"));
    ExprIdent::sptr id3(new ExprIdent("blarg"));
    ExprIdent::sptr id4(new ExprIdent("boo"));
    ExprIdent::sptr id6(new ExprIdent("feeb"));
    labels["feeb"] = LineNumberExpr(id5, 0);
    labels["boo"] = LineNumberExpr(id3, 0);
    labels["blarg"] = LineNumberExpr(id4, 31415);
    labels["able"] = LineNumberExpr(id3, 12345);
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
