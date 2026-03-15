add_executable(ascendir_parser
  main.cpp
  Parser.cpp
  Analyzer.cpp
  ExecutionContext.cpp
  Isa.cpp
  IsaRegistry.cpp
  IsaSequence.cpp
  IsaExecutor.cpp
  ExternalFunctionConfig.cpp
)

target_include_directories(ascendir_parser PRIVATE
  ${ascendir_parser_INCLUDE_DIR}
)

set_source_files_properties(ExternalFunctionConfig.cpp PROPERTIES COMPILE_FLAGS "-fexceptions")

target_link_libraries(ascendir_parser PRIVATE
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
)

if(BISHENGIR_DIR AND BISHENGIR_SOURCE_DIR)
  target_compile_definitions(ascendir_parser PRIVATE USE_BISHENGIR=1)
  link_bishengir(ascendir_parser)
endif()

llvm_update_compile_flags(ascendir_parser)
install(TARGETS ascendir_parser RUNTIME DESTINATION bin)
