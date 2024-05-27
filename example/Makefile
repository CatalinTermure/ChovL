.build_lib: stdlib.chv
	~/Code/lft/proiect/build/chovl stdlib.chv -o chovlstdlib.ll

.build_main: test_app.chv
	~/Code/lft/proiect/build/chovl test_app.chv -o test_app.ll

.link_main: .build_main .build_lib
	llvm-link-18 chovlstdlib.ll test_app.ll -o linked_test_app.ll -S

.compile_main: .link_main
	llc-18 linked_test_app.ll -o linked_test_app.s

assemble_main: .compile_main
	gcc linked_test_app.s -o linked_test_app

# for running the program, use ./linked_test_app