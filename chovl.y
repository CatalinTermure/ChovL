%{

#include <iostream>

extern int yylex();

void yyerror(const char *s) {
    std::cout << s << std::endl;
}

%}

%code requires {
#include "ast.h"
}

%union {
    int32_t i32;
    int64_t i64;
    float f32;
    double f64;
    chovl::ASTNode *node;
    chovl::TypeNode *type_id;
    chovl::ParameterNode *param;
    chovl::ParameterListNode *params;
    chovl::ASTAggregateNode *aggregate;
    chovl::Operator op;
    char *str;
    char chr;
}

%type <node> binary_expression constant function_definition function_body function_prototype function_declaration unary_expression expression block_node function_call
%type <aggregate> function_definition_list actual_param_list expression_list
%type <param> parameter
%type <params> formal_param_list non_void_formal_param_list
%type <type_id> type_identifier
%type <op> operator

%token OPEN_BRACK CLOSED_BRACK
%token OPEN_PAREN CLOSED_PAREN ARROW SEPARATOR COMMA
%token KW_FN KW_I32 KW_F32 KW_AS KW_CHAR
%token OP_ASSIGN
%token <str> IDENTIFIER
%token <i32> I32
%token <f32> F32
%token <chr> CHAR
%left <op> OP_ADD OP_SUB

%%

program : function_definition_list { chovl::AST($1).codegen(); }
        ;

function_definition_list : function_definition { $$ = new chovl::ASTListNode(); $$->push_back($1); }
                         | function_definition_list function_definition { $1->push_back($2);  $$ = $1; }
                         ;

function_definition : function_declaration function_body { $$ = new chovl::FunctionDefNode($1, $2); }
                    | function_prototype { $$ = $1; }
                    ;

function_prototype : function_declaration SEPARATOR { $$ = $1; }

function_declaration : KW_FN IDENTIFIER OPEN_PAREN formal_param_list CLOSED_PAREN ARROW type_identifier { $$ = new chovl::FunctionDeclNode($2, $4, $7); }
                     | KW_FN IDENTIFIER OPEN_PAREN formal_param_list CLOSED_PAREN { $$ = new chovl::FunctionDeclNode($2, $4, new chovl::TypeNode(chovl::Primitive::kNone)); }
                     | KW_FN type_identifier IDENTIFIER OPEN_PAREN formal_param_list CLOSED_PAREN { $$ = new chovl::FunctionDeclNode($3, $5, $2); }
                     ;

formal_param_list : formal_param_list COMMA parameter { $1->push_back($3); $$ = $1; }
                  | non_void_formal_param_list { $$ = $1; }
                  | { $$ = new chovl::ParameterListNode(); }
                  ;

non_void_formal_param_list : parameter { $$ = new chovl::ParameterListNode(); $$->push_back($1); }
                           ;

parameter : type_identifier IDENTIFIER { $$ = new chovl::ParameterNode($1, $2); }
          ;

type_identifier : KW_I32 { $$ = new chovl::TypeNode(chovl::Primitive::kI32); }
                | KW_F32 { $$ = new chovl::TypeNode(chovl::Primitive::kF32); }
                | KW_CHAR { $$ = new chovl::TypeNode(chovl::Primitive::kChar); }
                ;

function_body : OP_ASSIGN expression SEPARATOR { $$ = $2; }
              | block_node { $$ = $1; }
              ;

block_node : OPEN_BRACK expression_list CLOSED_BRACK { $$ = new chovl::BlockNode($2); }
           | OPEN_BRACK expression_list SEPARATOR CLOSED_BRACK { $$ = new chovl::BlockNode($2, true); }
           | OPEN_BRACK CLOSED_BRACK { $$ = new chovl::BlockNode(new chovl::ASTListNode()); }
           ;

expression_list : expression { $$ = new chovl::ASTListNode(); $$->push_back($1); }
                | expression_list SEPARATOR expression { $1->push_back($3); $$ = $1; }
                ;

expression : binary_expression { $$ = $1; }
           | unary_expression { $$ = $1; }
           ;

binary_expression : unary_expression operator unary_expression { $$ = new chovl::BinaryExprNode($2, $1, $3); }
                  | binary_expression operator unary_expression { $$ = new chovl::BinaryExprNode($2, $1, $3); }
                  ;

unary_expression : constant { $$ = $1; }
                 | constant KW_AS type_identifier { $$ = new chovl::CastOpNode($3, $1); }
                 | OPEN_PAREN expression CLOSED_PAREN { $$ = $2; }
                 | function_call { $$ = $1; }
                 | block_node { $$ = $1; }
                 ;

function_call : IDENTIFIER OPEN_PAREN actual_param_list CLOSED_PAREN { $$ = new chovl::FunctionCallNode($1, $3); }
              | IDENTIFIER OPEN_PAREN CLOSED_PAREN { $$ = new chovl::FunctionCallNode($1, new chovl::ASTListNode()); }
              ;

actual_param_list : expression { $$ = new chovl::ASTListNode(); $$->push_back($1); }
                  | actual_param_list COMMA expression { $1->push_back($3); $$ = $1; }
                  ;

constant : F32 { $$ = new chovl::F32Node($1); }
         | I32 { $$ = new chovl::I32Node($1); }
         | CHAR { $$ = new chovl::CharNode($1); }
         ;

operator : OP_ADD { $$ = $1; }
         | OP_SUB { $$ = $1; }
         ;

%%