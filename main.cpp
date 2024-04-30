#include <iostream>
#include <vector>
#include <stdexcept>

#include "ast.h"
#include "gen/parser.h"

extern int yylex(void);

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " file [-o output_file]" << '\n';
    return 1;
  }
  
  const char *input_file = argv[1];
  const char *output_file = [&]() -> const char* {
    if (argc == 4) {
      if (std::string(argv[2]) == "-o") {
        return argv[3];
      } else {
        std::cout << "Invalid argument";
        exit(1);
      }
    }
    return "a.ll";
  }();

  if (freopen(input_file, "r", stdin) == nullptr) {
    std::cerr << "Could not open input file: " << input_file << '\n';
    return 1;
  }
  if (freopen(output_file, "w", stdout) == nullptr) {
    std::cerr << "Could not open output file: " << output_file << '\n';
    return 1;
  }

  try {
    yyparse();
  } catch (std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
  return 0;
}