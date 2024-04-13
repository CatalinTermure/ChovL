#include <iostream>

#include "ast.h"
#include "gen/parser.h"

extern int yylex(void);

int main() {
  yyparse();
  return 0;
}