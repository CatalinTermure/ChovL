cmake_minimum_required(VERSION 3.27)
project(chovl VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(FLEX)
find_package(BISON)

include(CTest)
enable_testing()

set(GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gen)

FLEX_TARGET(chovl_lex chovl.l ${GENERATED_DIR}/lexer.cpp)
BISON_TARGET(chovl_yacc chovl.y ${GENERATED_DIR}/parser.cpp DEFINES_FILE ${GENERATED_DIR}/parser.h)
add_flex_bison_dependency(chovl_lex chovl_yacc)

add_library(parser STATIC
  ${FLEX_chovl_lex_OUTPUTS}
  ${BISON_chovl_yacc_OUTPUTS}
)

add_executable(chovl main.cpp)
target_link_libraries(chovl parser)

set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)

add_executable(chovl_test test_main.cpp)
target_link_libraries(chovl_test parser)

add_test(NAME chovl_test COMMAND chovl_test ${TEST_DIR}/test1)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
