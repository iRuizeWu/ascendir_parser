#include "Parser.h"
#include <iostream>
#include <string>
#include <vector>

void printUsage(const char *progName) {
  std::cerr << "Usage: " << progName << " [options] <mlir-file>\n"
            << "Options:\n"
            << "  --dump    Print function entries and IR source strings\n"
            << "  --help    Show this help message\n";
}

int main(int argc, char **argv) {
  bool dumpMode = false;
  std::string inputFile;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--dump") {
      dumpMode = true;
    } else if (arg == "--help") {
      printUsage(argv[0]);
      return 0;
    } else if (arg[0] != '-') {
      inputFile = arg;
    } else {
      std::cerr << "Unknown option: " << arg << "\n";
      printUsage(argv[0]);
      return 1;
    }
  }

  if (inputFile.empty()) {
    std::cerr << "Error: No input file specified\n";
    printUsage(argv[0]);
    return 1;
  }

  ascendir_parser::Parser parser;
  auto module = parser.parseFile(inputFile);
  if (!module) {
    return 1;
  }

  if (dumpMode) {
    auto functions = parser.extractFunctions(*module);
    std::cout << "=== Function Entries ===\n";
    for (const auto &func : functions) {
      std::cout << "Function: " << func.name << "\n";
    }
    std::cout << "\n=== IR Source Strings ===\n";
    for (const auto &func : functions) {
      std::cout << "--- " << func.name << " ---\n";
      std::cout << func.irSource << "\n\n";
    }
  } else {
    parser.printModule(*module);
  }

  return 0;
}
