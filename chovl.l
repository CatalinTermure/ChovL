%{
#include "parser.h"
%}

LETTER        [a-zA-Z_]
DIGIT         [0-9]

%%

"for"                               { return FOR_KW; }
"("                                 { return OPEN_PARENTHESIS; }
")"                                 { return CLOSED_PARENTHESIS; }
{LETTER}(?:{LETTER}|{DIGIT})*       { return VARIABLE_NAME; }
";"                                 { return SEPARATOR; }
"<="                                { return OP_LESS_EQ; }
">="                                { return OP_GREATER_EQ; }
"="                                 { return OP_ASSIGN; }
"<"                                 { return OP_LESS; }
">"                                 { return OP_GREATER; }
"&&"                                { return OP_AND; }
"||"                                { return OP_OR; }
"++"                                { return OP_INC; }
"--"                                { return OP_DEC; }
[1-9]{DIGIT}*                       { return INTEGER; }
.                                   ;

%%

int yywrap() {
    return 0;
}