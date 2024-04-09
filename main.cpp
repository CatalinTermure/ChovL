#include <iostream>

#include "gen/parser.h"

extern int yylex(void);

int main() {
  yyparse();
  return 0;
}