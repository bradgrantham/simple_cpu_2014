%{

#include <string>

#include "asm_yacc.tab.hpp"

int curLine = 1;

%}

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

\n|\r              { curLine++; return(NEWLINE); }
[ \t]*{COMMA}[ \t]*         return(COMMA);
[ \t]*         { }
"/""/".*        { }
                
{DASH}?{DIGIT}+     {
                     if(0)printf( "A decimal integer: %s (%d)\n", yytext,
                         atoi( yytext ) );
                     return(INTEGER);
                }

0[Xx]{HEXDIGIT}+   {
                     if(0)printf( "A hex integer: %s (%lx)\n", yytext,
                         strtol( yytext , NULL, 16) );
                     return(INTEGER);
                }
{QUOTE}{NOTQUOTE}*{QUOTE} {
                    std::string lit(yytext + 1, yytext + strlen(yytext) - 1);
                    if(0)printf("string literal: \"%s\"\n", lit.c_str());
                     return(STRINGLITERAL);
                }

hlt		return(HLT);
swapcc		return(SWAPCC);
rsr		return(RSR);
push		return(PUSH);
pop		return(POP);
jl		return(JL);
jmp		return(JMP);
jne		return(JNE);
sys		return(SYS);
and		return(AND);
or		return(OR);
xor		return(XOR);
not		return(NOT);
add		return(ADD);
adc		return(ADC);
sub		return(SUB);
mult		return(MULT);
div		return(DIV);
cmp		return(CMP);
xchg		return(XCHG);
mov		return(MOV);
moviu		return(MOVIU);
addiu		return(ADDIU);
addi		return(ADDI);
cmpiu		return(CMPIU);
jr		return(JR);
jsr		return(JSR);
shift		return(SHIFT);
load		return(LOAD);
store		return(STORE);

[Rr]{DIGIT}*     return(REGISTER); // ALSO store the number
[fF][pP]        return(REGISTER); // ALSO store the number, 5
[sS][pP]        return(REGISTER); // ALSO store the number, 6
[pP][cC]        return(REGISTER); // ALSO store the number, 7

{DOT}lr        return(DOT_LR);
{DOT}ll        return(DOT_LL);
{DOT}ar        return(DOT_AR);
{DOT}al        return(DOT_AL);

{DOT}org       return(DOT_ORG);
{DOT}byte       return(DOT_BYTE);
{DOT}short       return(DOT_SHORT);
{DOT}word       return(DOT_WORD);
{DOT}string       return(DOT_STRING);
{DOT}define       return(DOT_DEFINE);

{ID}            {
                    std::string label(yytext);
                    if(0)printf("identifier: \"%s\"\n", label.c_str());
                    return(IDENTIFIER);
                }
{ID}{COLON}     {
                    std::string label(yytext, yytext + strlen(yytext) - 1);
                    if(0)printf("label: \"%s\"\n", label.c_str());
                    return(LABEL);
                }
. { if(0)printf("\nsyntax error: %d\n", yytext[0]); }

%%

