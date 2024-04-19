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
"{"                                     { return OPEN_BRACK; }
"}"                                     { return CLOSED_BRACK; }
"fn"                                    { return KW_FN; }
"if"                                    { return KW_IF; }
"then"                                  { return KW_THEN; }
"else"                                  { return KW_ELSE; }
"as"                                    { return KW_AS; }
"char"                                  { return KW_CHAR; }
"i32"                                   { return KW_I32; }
"f32"                                   { return KW_F32; }
"="                                     { return OP_ASSIGN; }
"->"                                    { return ARROW; }
";"                                     { return SEPARATOR; }
","                                     { return COMMA; }
'.'                                     { yylval.chr = yytext[1]; return CHAR; }
'\\n'                                   { yylval.chr = 10; return CHAR; }
{LETTER}({LETTER}|{DIGIT})*             { yylval.str = (char*)malloc(strlen(yytext)); strcpy(yylval.str, yytext); return IDENTIFIER; }
0|([1-9]{DIGIT}*)                       { parse_int(yytext); return I32; }
(0|([1-9]{DIGIT}*))?\.{DIGIT}+          { yylval.f32 = atof(yytext); return F32; }
"+"                                     { yylval.op = chovl::Operator::kAdd; return OP_ADD; }
"-"                                     { yylval.op = chovl::Operator::kSub; return OP_SUB; }
"<"                                     { yylval.op = chovl::Operator::kLessThan; return OP_LT; }
"<="                                    { yylval.op = chovl::Operator::kLessEq; return OP_LEQ; }
">"                                     { yylval.op = chovl::Operator::kGreaterThan; return OP_GT; }
">="                                    { yylval.op = chovl::Operator::kGreaterEq; return OP_GEQ; }
"=="                                    { yylval.op = chovl::Operator::kEq; return OP_EQ; }
"!="                                    { yylval.op = chovl::Operator::kNotEq; return OP_NEQ; }
"||"                                    { yylval.op = chovl::Operator::kOr; return OP_OR; }
"&&"                                    { yylval.op = chovl::Operator::kAnd; return OP_AND; }
.                                       ;

%%

int yywrap() {
    return 1;
}