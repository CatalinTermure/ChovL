%{

#include <iostream>

extern int yylex();

void yyerror(const char *s) {
    std::cout << s << std::endl;
}

%}

%define parse.error verbose

%code requires {
#include "ast.h"
}

%union {
    int32_t i32;
    int64_t i64;
    float f32;
    double f64;
    chovl::ASTNode *node;
    chovl::AssignableNode *assignable;
    chovl::TypeNode *type_id;
    chovl::ParameterNode *param;
    chovl::ParameterListNode *params;
    chovl::ASTAggregateNode *aggregate;
    chovl::Operator op;
    chovl::PrimitiveType primitive;
    char *str;
    char chr;
}

%type <assignable> assignable_value
%type <node> cast_expression
%type <node> binary_expression additive_expression multiplicative_expression
%type <node> constant function_definition function_body function_prototype
%type <node> function_declaration primary_expression expression block_expression function_call statement
%type <node> block block_statement conditional_expression binary_conditional_expression
%type <aggregate> function_definition_list actual_param_list expression_list statement_list multi_expression
%type <param> parameter
%type <params> formal_param_list non_void_formal_param_list
%type <type_id> type_identifier
%type <primitive> primitive_type
%type <op> additive_operator multiplicative_operator conditional_operator conditional_composition_operator

%token OPEN_BRACK CLOSED_BRACK OPEN_SQ_BRACK CLOSED_SQ_BRACK
%token OPEN_PAREN CLOSED_PAREN ARROW SEPARATOR COMMA
%token KW_FN KW_I32 KW_F32 KW_AS KW_CHAR KW_IF KW_THEN KW_ELSE
%token OP_ASSIGN
%token <str> IDENTIFIER STRING_LITERAL
%token <i32> I32
%token <f32> F32
%token <chr> CHAR
%left <op> OP_OR OP_AND
%left <op> OP_LT OP_LEQ OP_GT OP_GEQ OP_EQ OP_NEQ
%left <op> OP_ADD OP_SUB
%left <op> OP_DIV OP_MUL OP_MOD

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
                   ;

function_declaration : KW_FN IDENTIFIER OPEN_PAREN formal_param_list CLOSED_PAREN ARROW type_identifier { $$ = new chovl::FunctionDeclNode($2, $4, $7); }
                     | KW_FN IDENTIFIER OPEN_PAREN formal_param_list CLOSED_PAREN { $$ = new chovl::FunctionDeclNode($2, $4, new chovl::TypeNode(chovl::Type(chovl::PrimitiveType::kNone))); }
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

primitive_type : KW_I32 { $$ = chovl::PrimitiveType::kI32; }
               | KW_F32 { $$ = chovl::PrimitiveType::kF32; }
               | KW_CHAR { $$ = chovl::PrimitiveType::kChar; }
               ;

type_identifier : primitive_type { $$ = new chovl::TypeNode(chovl::Type($1)); }
                | primitive_type OPEN_SQ_BRACK I32 CLOSED_SQ_BRACK { $$ = new chovl::TypeNode(chovl::Type($1, $3)); }
                ;

function_body : OP_ASSIGN expression SEPARATOR { $$ = $2; }
              | block { $$ = $1; }
              ;

block : block_expression { $$ = $1; }
      | block_statement { $$ = $1; }
      ;

block_expression : OPEN_BRACK expression_list CLOSED_BRACK { $$ = new chovl::BlockNode($2); }
                 ;

block_statement : OPEN_BRACK statement_list CLOSED_BRACK { $$ = new chovl::BlockNode($2, true); }
                | OPEN_BRACK CLOSED_BRACK { $$ = new chovl::BlockNode(new chovl::ASTListNode()); }
                ;

statement : expression SEPARATOR { $$ = $1; }
          | type_identifier IDENTIFIER SEPARATOR { $$ = new chovl::VariableDeclarationNode($1, $2, nullptr); }
          | type_identifier IDENTIFIER OP_ASSIGN expression SEPARATOR { $$ = new chovl::VariableDeclarationNode($1, $2, $4); }
          | assignable_value OP_ASSIGN expression SEPARATOR { $$ = new chovl::AssignmentNode($1, $3); }
          | assignable_value OP_ASSIGN OPEN_BRACK multi_expression CLOSED_BRACK SEPARATOR { $$ = new chovl::MultiAssignmentNode($1, $4); }
          | block_statement { $$ = $1; }
          | KW_IF primary_expression KW_THEN block_statement KW_ELSE block_statement { $$ = new chovl::CondStatementNode($2, $4, $6); }
          | KW_IF primary_expression KW_THEN block_statement { $$ = new chovl::CondStatementNode($2, $4, nullptr); }
          ;

statement_list : statement { $$ = new chovl::ASTListNode(); $$->push_back($1); }
               | statement_list statement { $1->push_back($2); $$ = $1; }
               ;

expression_list : expression { $$ = new chovl::ASTListNode(); $$->push_back($1); }
                | statement_list expression { $1->push_back($2); $$ = $1; }
                ;

expression : binary_expression { $$ = $1; }
           | conditional_expression { $$ = $1; }
           | binary_conditional_expression { $$ = $1; }
           ;

cast_expression : primary_expression { $$ = $1; }
                | primary_expression KW_AS type_identifier { $$ = new chovl::CastOpNode($3, $1); }
                ;

multiplicative_expression : cast_expression { $$ = $1; }
                          | multiplicative_expression multiplicative_operator cast_expression { $$ = new chovl::BinaryExprNode($2, $1, $3); }
                          ;

additive_expression : multiplicative_expression { $$ = $1; }
                    | additive_expression additive_operator multiplicative_expression { $$ = new chovl::BinaryExprNode($2, $1, $3); }
                    ;

binary_expression : additive_expression { $$ = $1; }
                  ;

assignable_value : IDENTIFIER { $$ = new chovl::VariableNode($1); }
                 | IDENTIFIER OPEN_SQ_BRACK expression CLOSED_SQ_BRACK { $$ = new chovl::ArrayAccessNode($1, $3); }
                 ;

primary_expression : constant { $$ = $1; }
                   | OPEN_PAREN expression CLOSED_PAREN { $$ = $2; }
                   | function_call { $$ = $1; }
                   | block_expression { $$ = $1; }
                   | assignable_value { $$ = $1; }
                   | KW_IF primary_expression KW_THEN primary_expression KW_ELSE primary_expression { $$ = new chovl::CondExprNode($2, $4, $6); }
                   ;

binary_conditional_expression : conditional_expression conditional_composition_operator conditional_expression { $$ = new chovl::BinaryExprNode($2, $1, $3); }
                              | binary_conditional_expression conditional_composition_operator conditional_expression { $$ = new chovl::BinaryExprNode($2, $1, $3); }
                              ;

conditional_expression : primary_expression conditional_operator primary_expression { $$ = new chovl::BinaryExprNode($2, $1, $3); }
                       ;

function_call : IDENTIFIER OPEN_PAREN actual_param_list CLOSED_PAREN { $$ = new chovl::FunctionCallNode($1, $3); }
              | IDENTIFIER OPEN_PAREN CLOSED_PAREN { $$ = new chovl::FunctionCallNode($1, new chovl::ASTListNode()); }
              ;

actual_param_list : expression { $$ = new chovl::ASTListNode(); $$->push_back($1); }
                  | multi_expression { $$ = $1; }
                  ;

multi_expression : expression COMMA expression { $$ = new chovl::ASTListNode(); $$->push_back($1); $$->push_back($3); }
                 | multi_expression COMMA expression { $1->push_back($3); $$ = $1; }
                 ;

constant : F32 { $$ = new chovl::F32Node($1); }
         | I32 { $$ = new chovl::I32Node($1); }
         | CHAR { $$ = new chovl::CharNode($1); }
         | STRING_LITERAL { $$ = new chovl::StringLiteralNode($1); }
         ;

additive_operator : OP_ADD { $$ = $1; }
                  | OP_SUB { $$ = $1; }
                  ;

multiplicative_operator : OP_DIV { $$ = $1; }
                        | OP_MUL { $$ = $1; }
                        | OP_MOD { $$ = $1; }
                        ;

conditional_operator : OP_LT  { $$ = $1; }
                     | OP_GT  { $$ = $1; }
                     | OP_LEQ { $$ = $1; }
                     | OP_GEQ { $$ = $1; }
                     | OP_EQ  { $$ = $1; }
                     | OP_NEQ { $$ = $1; }
                     ;

conditional_composition_operator : OP_OR  { $$ = $1; }
                                 | OP_AND { $$ = $1; }
                                 ;

%%