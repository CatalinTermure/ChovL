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
}

%type <node> binary_expression constant function_definition function_body function_prototype function_declaration
%type <aggregate> function_definition_list
%type <param> parameter
%type <params> parameter_list
%type <type_id> type_identifier
%type <op> operator

%token OPEN_PAREN CLOSED_PAREN ARROW SEPARATOR
%token KW_FN KW_I32 KW_F32
%token OP_ASSIGN
%token <str> IDENTIFIER
%token <i32> I32
%token <f32> F32
%left <op> OP_ADD

%%

program : function_definition_list { chovl::AST($1).codegen(); }
        ;

function_definition_list : function_definition { $$ = new chovl::ASTRootNode(); $$->push_back($1); }
                         | function_definition_list function_definition { $1->push_back($2);  $$ = $1; }
                         ;

function_definition : function_declaration function_body { $$ = new chovl::FunctionDefNode($1, $2); }
                    | function_prototype { $$ = $1; }
                    ;

function_prototype : function_declaration SEPARATOR { $$ = $1; }

function_declaration : KW_FN IDENTIFIER OPEN_PAREN parameter_list CLOSED_PAREN ARROW type_identifier { $$ = new chovl::FunctionDeclNode($2, $4, $7); }
                     | KW_FN IDENTIFIER OPEN_PAREN parameter_list CLOSED_PAREN { $$ = new chovl::FunctionDeclNode($2, $4, new chovl::TypeNode(chovl::Primitive::kNone)); }
                     | KW_FN type_identifier IDENTIFIER OPEN_PAREN parameter_list CLOSED_PAREN { $$ = new chovl::FunctionDeclNode($3, $5, $2); }
                     ;

parameter_list : parameter { $$ = new chovl::ParameterListNode(); $$->push_back($1); }
               | parameter_list parameter { $1->push_back($2); $$ = $1; }

parameter : type_identifier IDENTIFIER { $$ = new chovl::ParameterNode($1, $2); }
          ;

type_identifier : KW_I32 { $$ = new chovl::TypeNode(chovl::Primitive::kI32); }
                | KW_F32 { $$ = new chovl::TypeNode(chovl::Primitive::kF32); }
                ;

function_body : OP_ASSIGN binary_expression SEPARATOR { $$ = $2; }
              | OP_ASSIGN constant SEPARATOR { $$ = $2; }
              ;

binary_expression : constant operator constant { $$ = new chovl::BinaryExprNode($2, $1, $3); }
                  | binary_expression operator constant { $$ = new chovl::BinaryExprNode($2, $1, $3); }
                  ;

constant : F32 { $$ = new chovl::F32Node($1); }
         | I32 { $$ = new chovl::I32Node($1); }
         ;

operator : OP_ADD { $$ = $1; }

%%