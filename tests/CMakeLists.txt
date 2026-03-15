# Test sources
set(TEST_SOURCES
  test_parser.cpp
  test_isa_sequence.cpp
  test_isa_executor.cpp
)

# Create test executable
add_executable(ascendir_tests ${TEST_SOURCES}
  ${ascendir_parser_SRC_DIR}/src/Parser.cpp
  ${ascendir_parser_SRC_DIR}/src/Analyzer.cpp
  ${ascendir_parser_SRC_DIR}/src/ExecutionContext.cpp
  ${ascendir_parser_SRC_DIR}/src/Isa.cpp
  ${ascendir_parser_SRC_DIR}/src/IsaRegistry.cpp
  ${ascendir_parser_SRC_DIR}/src/IsaSequence.cpp
  ${ascendir_parser_SRC_DIR}/src/IsaExecutor.cpp
  ${ascendir_parser_SRC_DIR}/src/ExternalFunctionConfig.cpp
)

target_include_directories(ascendir_tests PRIVATE
  ${ascendir_parser_INCLUDE_DIR}
)

set_source_files_properties(${ascendir_parser_SRC_DIR}/src/ExternalFunctionConfig.cpp PROPERTIES COMPILE_FLAGS "-fexceptions")

set_target_properties(ascendir_tests PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)

target_link_libraries(ascendir_tests PRIVATE
  MLIRIR
  MLIRParser
  MLIRSupport
  MLIRFuncDialect
  MLIRArithDialect
  MLIRMemRefDialect
  MLIRLLVMDialect
  MLIRControlFlowDialect
  MLIRSCFDialect
  LLVMSupport
  yaml-cpp
  GTest::gtest_main
  GTest::gtest
  pthread
)

llvm_update_compile_flags(ascendir_tests)

# Register tests with ctest
add_test(NAME ascendir_unit_tests COMMAND ascendir_tests)
