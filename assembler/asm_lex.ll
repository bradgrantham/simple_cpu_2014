%{
#include <string>

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

\n              { curLine++; }
[ \t]*{COMMA}[ \t]*         { printf("comma\n"); }
[ \t]*         { /* discard whitespace */ }
"/""/".*        {
                    /* discard comments printf( "Comment: %s\n", yytext + 2); */
                }
                
{DASH}?{DIGIT}+     {
                     printf( "A decimal integer: %s (%d)\n", yytext,
                         atoi( yytext ) );
                }

0[Xx]{HEXDIGIT}+   {
                     printf( "A hex integer: %s (%x)\n", yytext,
                         strtol( yytext , NULL, 16) );
                }
{QUOTE}{NOTQUOTE}*{QUOTE} {
                    std::string lit(yytext + 1, yytext + strlen(yytext) - 1);
                    printf("string literal: \"%s\"\n", lit.c_str());
                }

hlt             { printf("instruction HLT\n"); }
swapcc          { printf("instruction SWAPCC\n"); }
rsr             { printf("instruction RSR\n"); }
push            { printf("instruction PUSh\n"); }
pop             { printf("instruction POP\n"); }
jl              { printf("instruction JL\n"); }
jmp             { printf("instruction JMP\n"); }
jne             { printf("instruction JNE\n"); }
sys             { printf("instruction SYS\n"); }
and             { printf("instruction AND\n"); }
or              { printf("instruction OR\n"); }
xor             { printf("instruction XOR\n"); }
not             { printf("instruction NOT\n"); }
add             { printf("instruction ADD\n"); }
adc             { printf("instruction ADC\n"); }
sub             { printf("instruction SUB\n"); }
mult            { printf("instruction MULT\n"); }
div             { printf("instruction DIV\n"); }
cmp             { printf("instruction CMP\n"); }
xchg            { printf("instruction XCHG\n"); }
mov             { printf("instruction MOV\n"); }
moviu           { printf("instruction MOVIU\n"); }
addiu           { printf("instruction ADDIU\n"); }
addi            { printf("instruction ADDI\n"); }
cmpiu           { printf("instruction CMPIU\n"); }
shift           { printf("instruction SHIFT\n"); }
jr              { printf("instruction JR\n"); }
jsr             { printf("instruction JSR\n"); }
[Rr]{DIGIT}             { printf("register %d\n", atoi(yytext + 1)); }
[pP][cC]                      { printf("register PC\n"); }
[fF][pP]                      { printf("register FP\n"); }
[sS][pP]                      { printf("register SP\n"); }
shift{DOT}{ID}           { printf("instruction SHIFT, modifier: %s\n", yytext + 6); }
load{DOT}{ID}            { printf("instruction LOAD, modifier: %s\n", yytext + 5); }
store{DOT}{ID}           { printf("instruction STORE, modifier: %s\n", yytext + 6); }

{DOT}{ID}       {
                    std::string dir(yytext + 1);
                    printf("directive: \"%s\"\n", dir.c_str());
                }

{ID}            {
                    std::string label(yytext);
                    printf("identifier: \"%s\"\n", label.c_str());
                }
{ID}{COLON}     {
                    std::string label(yytext, yytext + strlen(yytext) - 1);
                    printf("label: \"%s\"\n", label.c_str());
                }
. { printf("\nBLARG \"%s\"\n\n", yytext); }

%%

         int main( int argc, char **argv )
             {
             if ( argc > 1 )
                     yyin = fopen( argv[1], "r" );
             else
                     yyin = stdin;
     
             yylex();
             }


/*

byte = 'byte'
word = 'word'
short = 'short'
string = 'string'

size_modifier = ( byte | word | short )

definedirective = dot 'define' whitespaceplus identifier whitespaceplus number
(* store an identifier with value number *)

memdirective = dot size_modifier whitespaceplus number { comma_delim number }
(* maybe pad address to size; set any labels; store numbers, checking sizes, incrementing by address size *)

stringdirective = dot 'string' whitespaceplus stringliteral
(* set any labels ; store string, incrementing address by size of string *)

directive = ( orgdirective | definedirective | memdirective | stringdirective )

shift_type = ( rl | al | rr | ar )

instruction_direct = ( halt )

instruction_rx = ( swapcc | rsr | push | pop ) whitespaceplus register

instruction_imm = ( jl | jmp | jne | sys ) whitespaceplus ( number | identifier )

instruction_rxry = ( and | or | xor | not | add | adc | sub | mult | div | cmp | xchg | mov ) whitespaceplus register comma_delim register

instruction_rximm = ( moviu | addiu | addi | cmpiu | jr | jsr ) whitespaceplus register comma_delim (number | identifier)

instruction_rximm_size = ( shift ) dot shift_type whitespaceplus register comma_delim ( number | identifier )

instruction_rxryimm = ( load | store ) dot size_modifier whitespaceplus register comma_delim register COMMAspace ( number | identifier )

(* pad address to 4; set any labels ; store instruction to set later *)
(* go through instructions, evaluate data parameters, check size of data, store *)

*/
