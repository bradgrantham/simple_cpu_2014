OPT     = -O3 -DNDEBUG
CXXFLAGS=-I/opt/local/include/ -Wall $(OPT) --std=c++11 
CXX = clang++

asm: lex.yy.o asm_yacc.tab.o parsing.o
	$(CXX) $(CXXFLAGS) -o $@  $^ -ll

parsing.o: parsing.h

lex.yy.o: asm_yacc.tab.h

asm_yacc.tab.o: parsing.h

lex.yy.cpp: asm_lex.ll asm_yacc.tab.h 
	flex  -o lex.yy.cpp asm_lex.ll

asm_yacc.tab.cpp asm_yacc.tab.h: asm_yacc.ypp
	bison -d -v asm_yacc.ypp

clean:
	$(RM) lex.yy.cpp asm_yacc.tab.cpp asm_yacc.tab.h *.o
