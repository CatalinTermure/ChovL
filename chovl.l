%{
#include "parser.h"
%}

LETTER        [a-zA-Z_]
DIGIT         [0-9]
WHITESPACE    [ \t\n]

%%

0|([1-9]{DIGIT}*)                       { yylval.i = atoi(yytext); return INTEGER; }
(0|([1-9]{DIGIT}*))?\.{DIGIT}+          { yylval.f = atof(yytext); return FLOAT;   }
.                                       ;

%%

int yywrap() {
    return 1;
}