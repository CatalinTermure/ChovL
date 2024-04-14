#include <cstdio>
#include <cstdlib>

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
  char llvm_as_command[2400];
  for (int i = 1; i < argc; i++) {
    sprintf(input_file_name, "%s.out", argv[i]);
    sprintf(output_file_name, "%s.asm", argv[i]);

    // change llvm-as-18 to whichever llvm assembler you have
    sprintf(llvm_as_command, "llvm-as-18 -o %s %s", output_file_name,
            input_file_name);

    int result = system(llvm_as_command);

    if (result != 0) {
      overall_result = 1;
    }
  }

  return overall_result;
}