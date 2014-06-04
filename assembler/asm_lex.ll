%{

#include <string>

#include "parsing.h"
#include "asm_yacc.tab.hpp"

#pragma clang diagnostic ignored "-Wdeprecated-register"

#define YY_NO_UNPUT

int curLine = 1;

void yyerror(const char *);

void 
yyerrorf(const char *fmt, ...)
{
    va_list args;
    char dummy[1024];

    va_start(args, fmt);
    vsprintf(dummy, fmt, args);
    va_end(args);
    
    yyerror(dummy);
}


%}

%option nounput

DIGIT		[0-9]
QUOTE		["]
DASH		[-]
DOT		[.]
NOTQUOTE	[^"]
HEXDIGIT	[0-9a-fA-F]
COLON		[:]
COMMA           [,]
ID		[a-zA-Z_][a-zA-Z_0-9]*

%%

\n|\r                   { curLine++; return NEWLINE; }
<<EOF>>                 {
                            static bool saweof = false;
                            if(!saweof) {
                                saweof = true;
                                return NEWLINE;
                            } else {
                                return 0;
                            }
                        }
[ \t]*{COMMA}[ \t]*     return(COMMA);
[ \t]*                  { }
"/""/".*                { }
                
{DASH}?{DIGIT}+         {
                            yylval.i = atoi(yytext);
                            return(INTEGER);
                        }

0[Xx]{HEXDIGIT}+        {
                            yylval.i = strtol(yytext, NULL, 16);
                            return INTEGER;
                        }
{QUOTE}{NOTQUOTE}*{QUOTE} {
                            yylval.str = new std::string(yytext + 1, yytext + strlen(yytext) - 1);
                            return(STRINGLITERAL);
                        }

hlt		        return(HLT);
swapcc		        return(SWAPCC);
rsr		        return(RSR);
push		        return(PUSH);
pop		        return(POP);
jl		        return(JL);
jmp		        return(JMP);
jne		        return(JNE);
sys		        return(SYS);
and		        return(AND);
or		        return(OR);
xor		        return(XOR);
not		        return(NOT);
add		        return(ADD);
adc		        return(ADC);
sub		        return(SUB);
mult		        return(MULT);
div		        return(DIV);
cmp		        return(CMP);
xchg		        return(XCHG);
mov		        return(MOV);
moviu		        return(MOVIU);
addiu		        return(ADDIU);
addi		        return(ADDI);
cmpiu		        return(CMPIU);
jr		        return(JR);
jsr		        return(JSR);
shift		        return(SHIFT);
load		        return(LOAD);
store		        return(STORE);

[Rr]{DIGIT}*            { yylval.i = yytext[1] - '0'; return(REGISTER); }
[fF][pP]                { yylval.i = 5; return(REGISTER); }
[sS][pP]                { yylval.i = 6; return(REGISTER); }
[pP][cC]                { yylval.i = 7; return(REGISTER); }

{DOT}rl                 return(DOT_RL);
{DOT}ll                 return(DOT_LL);
{DOT}ra                 return(DOT_RA);
{DOT}la                 return(DOT_LA);

{DOT}org                return(DOT_ORG);
{DOT}byte               return(DOT_BYTE);
{DOT}short              return(DOT_SHORT);
{DOT}word               return(DOT_WORD);
{DOT}string             return(DOT_STRING);
{DOT}define             return(DOT_DEFINE);

{ID}                    {
                            yylval.str = new std::string(yytext);
                            return(IDENTIFIER);
                        }
{ID}{COLON}             {
                            yylval.str = new std::string(yytext, yytext + strlen(yytext) - 1);
                            return(LABEL);
                        }
.                       { yyerrorf("unexpected character: %02X, '%c'\n", yytext[0], yytext[0]); yyterminate(); }

%%

