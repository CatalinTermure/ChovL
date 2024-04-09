%{
#include "parser.h"
%}

LETTER        [a-zA-Z_]
DIGIT         [0-9]
WHITESPACE    [ \t\n]

%%

0|([1-9]{DIGIT}*)                       { yylval = atoi(yytext); return INTEGER; }
.                                       ;

%%

int yywrap() {
    return 1;
}