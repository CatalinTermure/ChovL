%{
#include "parser.h"

#include <string.h>
#include <limits.h>

void parse_int(const char *str) {
    int val = atoi(str);
    yylval.i32 = val;
}
%}

LETTER        [a-zA-Z_]
DIGIT         [0-9]
WHITESPACE    [ \t\n]

%%

"("                                     { return OPEN_PAREN; }
")"                                     { return CLOSED_PAREN; }
"extern"                                { return KW_EXTERN; }
"fn"                                    { return KW_FN; }
"i32"                                   { return KW_I32; }
"f32"                                   { return KW_F32; }
"="                                     { return OP_ASSIGN; }
"->"                                    { return ARROW; }
";"                                     { return SEPARATOR; }
{LETTER}({LETTER}|{DIGIT})*             { yylval.str = (char*)malloc(strlen(yytext)); strcpy(yylval.str, yytext); return IDENTIFIER; }
0|([1-9]{DIGIT}*)                       { parse_int(yytext); return I32; }
(0|([1-9]{DIGIT}*))?\.{DIGIT}+          { yylval.f32 = atof(yytext); return F32; }
"+"                                     { yylval.op = chovl::Operator::kAdd; return OP_ADD; }
.                                       ;

%%

int yywrap() {
    return 1;
}