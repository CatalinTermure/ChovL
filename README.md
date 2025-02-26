# ChovL

## Overview

ChovL is a simple programming language made for the purpose of playing with the LLVM compiler toolchain. This was also used as a school project.

The language is structured into:
* A parser - using [Flex](https://github.com/westes/flex), source is chovl.l
* A lexer - using [Bison](https://www.gnu.org/software/bison/), source is chovl.y
* An LLVM IR code generator - written in C++ using the LLVM support libraries, source is in src/

The parser reads the source file(s) and sends them to the lexer. The lexer uses a grammar definition in BNF(Backus-Naur form) to create the AST(Abstract Syntax Tree). After we build the AST, we use it to generate the LLVM IR code. This can then be compiled to Assembly code using the LLVM compiler - `llc`.

## Dependencies

The project uses CMake as a build system.

To build the compiler, you will need the following libraries:
* Flex
* Bison
* LLVM - tested with LLVM-18
