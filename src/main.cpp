#include "Parser.h"
#include "InstructionExecutor.h"
#include "InstructionRegistry.h"
#include "Instruction.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include <string>
#include <vector>

void printUsage(const char *progName) {
  std::cerr << "Usage: " << progName << " [options] <mlir-file>\n"
            << "Options:\n"
            << "  --dump      Print function entries and IR source strings\n"
            << "  --analyze   Analyze unregistered operations\n"
            << "  --printops  Print all operations line by line\n"
            << "  --printops-detailed  Print all operations with detailed format\n"
            << "  --simulate  Execute the MLIR program\n"
            << "  --dump-sequence  Dump instruction sequence before execution\n"
            << "  --verbose   Show execution details\n"
            << "  --help      Show this help message\n";
}

int main(int argc, char **argv) {
  bool dumpMode = false;
  bool analyzeMode = false;
  bool printOpsMode = false;
  bool printOpsDetailedMode = false;
  bool simulateMode = false;
  bool dumpSequenceMode = false;
  bool verboseMode = false;
  std::string inputFile;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--dump") {
      dumpMode = true;
    } else if (arg == "--analyze") {
      analyzeMode = true;
    } else if (arg == "--printops") {
      printOpsMode = true;
    } else if (arg == "--printops-detailed") {
      printOpsDetailedMode = true;
    } else if (arg == "--simulate") {
      simulateMode = true;
    } else if (arg == "--dump-sequence") {
      dumpSequenceMode = true;
    } else if (arg == "--verbose") {
      verboseMode = true;
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

  if (analyzeMode) {
    ascendir_parser::analyzeUnregisteredOps(*module);
  } else if (printOpsDetailedMode) {
    parser.printOps(*module, true);
  } else if (printOpsMode) {
    parser.printOps(*module, false);
  } else if (dumpMode) {
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
  } else if (simulateMode) {
    ascendir_parser::registerAllInstructions();
    
    ascendir_parser::InstructionExecutor executor;
    executor.load(*module);
    
    if (dumpSequenceMode) {
      executor.getSequence().dump();
      std::cout << "\n";
    }
    
    if (verboseMode) {
      std::cout << "Starting simulation...\n\n";
      while (!executor.isHalted()) {
        uint64_t pc = executor.getCurrentPC();
        uint64_t startCycle = executor.getContext().getCycle();
        auto& seq = executor.getSequence();
        const ascendir_parser::Instruction* inst = seq.getInstructionByPCConst(pc);
        std::string desc = seq.getInstructionDescription(pc);
        
        std::cout << "[cycle=" << startCycle << "] PC " << pc << ": " << desc << "\n";
        
        executor.executeNext();
        
        uint64_t endCycle = executor.getContext().getCycle();
        std::cout << "[cycle=" << endCycle << "] retired\n\n";
      }
      std::cout << "Simulation completed.\n";
      std::cout << "Final PC: " << executor.getCurrentPC() << "\n";
      std::cout << "Total cycles: " << executor.getContext().getCycle() << "\n";
    } else {
      executor.run();
      std::cout << "Simulation completed.\n";
    }
  } else {
    parser.printModule(*module);
  }

  return 0;
}
