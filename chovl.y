%{

#include <iostream>

extern int yylex();

void yyerror(const char *s) {
    std::cout << s << std::endl;
}

%}

%token FOR_KW OPEN_PARENTHESIS CLOSED_PARENTHESIS VARIABLE_NAME SEPARATOR OP_LESS_EQ OP_GREATER_EQ OP_ASSIGN OP_LESS OP_GREATER OP_AND OP_OR OP_INC OP_DEC INTEGER

%%

for_loop : FOR_KW OPEN_PARENTHESIS assignment SEPARATOR condition SEPARATOR statement CLOSED_PARENTHESIS { std::cout << "Recognized for loop"; }
         ;

assignment : VARIABLE_NAME OP_ASSIGN value
           ;

value : INTEGER
      | VARIABLE_NAME
      ;

condition : condition logical_operator condition
          | value comparison_operator value
          ;

logical_operator : OP_AND
                 | OP_OR
                 ;

comparison_operator : OP_LESS
                    | OP_LESS_EQ
                    | OP_GREATER
                    | OP_GREATER_EQ
                    ;

statement : assignment
          | VARIABLE_NAME OP_INC
          | VARIABLE_NAME OP_DEC
          ;

%%