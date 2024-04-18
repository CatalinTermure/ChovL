cmake_minimum_required(VERSION 3.27)
project(chovl VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)
find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})
llvm_map_components_to_libnames(llvm_libs support core irreader)

include(CTest)
enable_testing()

set(GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gen)

FLEX_TARGET(chovl_lex chovl.l ${GENERATED_DIR}/lexer.cpp)
BISON_TARGET(chovl_yacc chovl.y ${GENERATED_DIR}/parser.cpp DEFINES_FILE ${GENERATED_DIR}/parser.h)
add_flex_bison_dependency(chovl_lex chovl_yacc)

include_directories(include/)

add_library(parser STATIC
  ${FLEX_chovl_lex_OUTPUTS}
  ${BISON_chovl_yacc_OUTPUTS}
  src/ast.cpp
  src/scope.cpp
  src/context.cpp
)
target_link_libraries(parser ${llvm_libs})

add_executable(chovl main.cpp)
target_link_libraries(chovl parser)

set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)

add_executable(chovl_diff_test diff_test_main.cpp)
target_link_libraries(chovl_diff_test parser)

add_executable(chovl_validation_test validation_test_main.cpp)

file(GLOB TEST_FILES ${TEST_DIR}/*.chv)
foreach(TEST_FILE ${TEST_FILES})
  get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
  add_test(NAME ${TEST_NAME}_diff COMMAND chovl_diff_test ${TEST_DIR}/${TEST_NAME})
  add_test(NAME ${TEST_NAME}_validation COMMAND chovl_validation_test ${TEST_DIR}/${TEST_NAME})
  set_tests_properties(${TEST_NAME}_diff PROPERTIES TIMEOUT 30)
  set_tests_properties(${TEST_NAME}_validation PROPERTIES TIMEOUT 30)
  set_tests_properties(${TEST_NAME}_validation PROPERTIES DEPENDS ${TEST_NAME}_diff)
endforeach()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
