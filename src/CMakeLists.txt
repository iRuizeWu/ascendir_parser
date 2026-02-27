add_executable(ascendir_parser
  main.cpp
  Parser.cpp
)

target_include_directories(ascendir_parser PRIVATE
  ${ascendir_parser_INCLUDE_DIR}
)

target_link_libraries(ascendir_parser PRIVATE
  MLIRIR
  MLIRParser
  MLIRSupport
  MLIRFuncDialect
  MLIRArithDialect
  MLIRMemRefDialect
)

if(BISHENGIR_DIR AND BISHENGIR_SOURCE_DIR)
  target_compile_definitions(ascendir_parser PRIVATE USE_BISHENGIR=1)
  link_bishengir(ascendir_parser)
endif()

llvm_update_compile_flags(ascendir_parser)
install(TARGETS ascendir_parser RUNTIME DESTINATION bin)
