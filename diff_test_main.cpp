#include <cstdio>

#include "gen/parser.h"

extern int yylex();

int CheckFiles(const char* test_name, FILE* output_file, FILE* gold_file) {
  int output_char = fgetc(output_file);
  int gold_char = fgetc(gold_file);
  int line = 1;
  int col = 1;
  while (output_char != EOF && gold_char != EOF) {
    if (output_char == '\n' || output_char == ' ') {
      output_char = fgetc(output_file);
      continue;
    }
    if (gold_char == '\n' || gold_char == ' ') {
      gold_char = fgetc(gold_file);
      continue;
    }
    if (output_char != gold_char) {
      fprintf(stderr,
              "%s: Mismatch at line %d, col %d: expected '%c', got '%c'\n",
              test_name, line, col, gold_char, output_char);
      return 1;
    }
    if (output_char == '\n') {
      line++;
      col = 1;
    } else {
      col++;
    }
    output_char = fgetc(output_file);
    gold_char = fgetc(gold_file);
  }
  if (output_char != EOF) {
    while (output_char != EOF) {
      if (output_char != ' ' && output_char != '\n') {
        fprintf(stderr, "%s: Output file is longer than gold file\n",
                test_name);
        return 1;
      }
      output_char = fgetc(output_file);
    }
  } else if (gold_char != EOF) {
    while (gold_char != EOF) {
      if (gold_char != ' ' && gold_char != '\n') {
        fprintf(stderr, "%s: Gold file is longer than output file\n",
                test_name);
        return 1;
      }
      gold_char = fgetc(gold_file);
    }
    return 1;
  }

  return 0;
}

#define CHECK_OPEN(file, name, file_name, overall_result)             \
  if ((file) == NULL) {                                               \
    fprintf(stderr, "Could not open %s file: %s\n", name, file_name); \
    (overall_result) = 1;                                             \
  }

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf(
        "Usage: %s <test_name1> [<test_name2>] [<test_name3>] ... "
        "[<test_nameN>]\n",
        argv[0]);
    return 1;
  }

  int overall_result = 0;

  char input_file_name[1024];
  char output_file_name[1024];
  char gold_file_name[1024];
  for (int i = 1; i < argc; i++) {
    sprintf(input_file_name, "%s.chv", argv[i]);
    sprintf(output_file_name, "%s.ll", argv[i]);
    sprintf(gold_file_name, "%s.gold", argv[i]);

    FILE* gold_file = fopen(gold_file_name, "r");

    CHECK_OPEN(gold_file, "gold", gold_file_name, overall_result);

    freopen(input_file_name, "r", stdin);
    freopen(output_file_name, "w", stdout);

    yyparse();

    fclose(stdin);
    fclose(stdout);

    FILE* output_file = fopen(output_file_name, "r");
    if (output_file == NULL) {
      fprintf(stderr, "Could not open output file for reading: %s\n",
              output_file_name);
      return 1;
    }

    int result = CheckFiles(input_file_name, output_file, gold_file);

    fclose(output_file);
    fclose(gold_file);

    if (result != 0) {
      overall_result = 1;
    }
  }

  return overall_result;
}