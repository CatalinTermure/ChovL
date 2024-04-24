#include <iostream>
#include <stdexcept>

#include "ast.h"
#include "gen/parser.h"

extern int yylex(void);

int main() {
  try {
    yyparse();
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
  return 0;
}