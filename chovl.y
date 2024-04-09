%{

#include "ast.h"

#include <iostream>

extern int yylex();

void yyerror(const char *s) {
    std::cout << s << std::endl;
}

%}

%token INTEGER

%%

constant : INTEGER { std::cout << $1 << std::endl; }
         ;

%%