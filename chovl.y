%{

#include "ast.h"

#include <iostream>

extern int yylex();

void yyerror(const char *s) {
    std::cout << s << std::endl;
}

%}

%union {
    int i;
    float f;
}

%token <i> INTEGER
%token <f> FLOAT
%type <i> integer_constant
%type <f> float_constant

%%

constant : integer_constant
         | float_constant
         ;



integer_constant : INTEGER { $$ = $1; std::cout << $$ << std::endl; }
                 ;

float_constant   : FLOAT   { $$ = $1; std::cout << $$ << std::endl; }
                 ;

%%