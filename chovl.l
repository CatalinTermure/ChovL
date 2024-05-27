%{
#include "parser.h"

#include <string.h>
#include <limits.h>

%}

LETTER        [a-zA-Z_]
DIGIT         [0-9]

%%

"("                                     { return OPEN_PAREN; }
")"                                     { return CLOSED_PAREN; }
"{"                                     { return OPEN_BRACK; }
"}"                                     { return CLOSED_BRACK; }
"["                                     { return OPEN_SQ_BRACK; }
"]"                                     { return CLOSED_SQ_BRACK; }
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
"&"                                     { return REF; }
'.'                                     { yylval.chr = yytext[1]; return CHAR; }
'\\n'                                   { yylval.chr = 10; return CHAR; }
{LETTER}({LETTER}|{DIGIT})*             {
                                            yylval.str = (char*)malloc(strlen(yytext) + 1);
                                            strcpy(yylval.str, yytext);
                                            return IDENTIFIER;
                                        }
-?(0|([1-9]{DIGIT}*))                   { yylval.i32 = atoi(yytext); return I32; }
-?((0|([1-9]{DIGIT}*))?\.{DIGIT}+)      { yylval.f32 = atof(yytext); return F32; }
\".*\"                                  {
                                            int len = strlen(yytext) - 1;
                                            yylval.str = (char*)malloc(len + 1);
                                            strcpy(yylval.str, yytext + 1);
                                            yylval.str[len - 1] = 0; return STRING_LITERAL;
                                        }
"+"                                     { yylval.op = chovl::Operator::kAdd; return OP_ADD; }
"-"                                     { yylval.op = chovl::Operator::kSub; return OP_SUB; }
"/"                                     { yylval.op = chovl::Operator::kDiv; return OP_DIV; }
"%"                                     { yylval.op = chovl::Operator::kMod; return OP_MOD; }
"*"                                     { yylval.op = chovl::Operator::kMul; return OP_MUL; }
"<"                                     { yylval.op = chovl::Operator::kLessThan; return OP_LT; }
"<="                                    { yylval.op = chovl::Operator::kLessEq; return OP_LEQ; }
">"                                     { yylval.op = chovl::Operator::kGreaterThan; return OP_GT; }
">="                                    { yylval.op = chovl::Operator::kGreaterEq; return OP_GEQ; }
"=="                                    { yylval.op = chovl::Operator::kEq; return OP_EQ; }
"!="                                    { yylval.op = chovl::Operator::kNotEq; return OP_NEQ; }
"||"                                    { yylval.op = chovl::Operator::kOr; return OP_OR; }
"&&"                                    { yylval.op = chovl::Operator::kAnd; return OP_AND; }
\/\/[^\n]*                              ;
\n                                      ;
.                                       ;

%%

int yywrap() {
    return 1;
}