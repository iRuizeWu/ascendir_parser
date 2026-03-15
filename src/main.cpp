#include "Parser.h"
#include "ExecutionUnit.h"
#include "IsaExecutor.h"
#include "IsaRegistry.h"
#include "ExternalFunctionConfig.h"
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
            << "  --sync      Run in synchronous mode (wait for each operation)\n"
            << "  --func-config <file>  Load external function config from YAML file\n"
            << "  --func-cycle <name=latency>  Set cycle for a specific function\n"
            << "  --dump-config  Dump external function configuration\n"
            << "  --help      Show this help message\n";
}

bool parseFuncCycleArg(const std::string& arg, std::string& funcName, uint64_t& latency) {
  size_t eqPos = arg.find('=');
  if (eqPos == std::string::npos) {
    return false;
  }
  funcName = arg.substr(0, eqPos);
  std::string latencyStr = arg.substr(eqPos + 1);
  
  if (latencyStr.empty()) {
    return false;
  }
  
  for (char c : latencyStr) {
    if (c < '0' || c > '9') {
      return false;
    }
  }
  
  latency = 0;
  for (char c : latencyStr) {
    latency = latency * 10 + (c - '0');
  }
  return true;
}

int main(int argc, char **argv) {
  bool dumpMode = false;
  bool analyzeMode = false;
  bool printOpsMode = false;
  bool printOpsDetailedMode = false;
  bool simulateMode = false;
  bool dumpSequenceMode = false;
  bool verboseMode = false;
  bool syncMode = false;
  bool dumpConfigMode = false;
  std::string inputFile;
  std::string configFile;
  std::vector<std::pair<std::string, uint64_t>> funcCycles;

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
    } else if (arg == "--sync") {
      syncMode = true;
    } else if (arg == "--dump-config") {
      dumpConfigMode = true;
    } else if (arg == "--func-config") {
      if (i + 1 < argc) {
        configFile = argv[++i];
      } else {
        std::cerr << "Error: --func-config requires a file path\n";
        return 1;
      }
    } else if (arg == "--func-cycle") {
      if (i + 1 < argc) {
        std::string cycleArg = argv[++i];
        std::string funcName;
        uint64_t latency;
        if (parseFuncCycleArg(cycleArg, funcName, latency)) {
          funcCycles.push_back({funcName, latency});
        } else {
          std::cerr << "Error: Invalid --func-cycle format. Use: name=latency\n";
          return 1;
        }
      } else {
        std::cerr << "Error: --func-cycle requires an argument\n";
        return 1;
      }
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

  auto& extFuncConfig = ascendir_parser::ExternalFunctionConfig::getGlobalConfig();
  
  if (!configFile.empty()) {
    if (!extFuncConfig.loadFromFile(configFile)) {
      std::cerr << "Warning: Failed to load config file: " << configFile << "\n";
    }
  }
  
  for (const auto& fc : funcCycles) {
    extFuncConfig.setFunctionCycle(fc.first, fc.second);
  }
  
  if (dumpConfigMode) {
    extFuncConfig.dump();
    return 0;
  }

  if (inputFile.empty()) {
    std::cerr << "Error: No input file specified\n";
    printUsage(argv[0]);
    return 1;
  }
  
  if (simulateMode && verboseMode) {
    if (!configFile.empty()) {
      llvm::outs() << "Loaded external function config from: " << configFile << "\n";
    }
    for (const auto& fc : funcCycles) {
      llvm::outs() << "Set function cycle: " << fc.first << " = " << fc.second << "\n";
    }
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
    ascendir_parser::registerAllIsas();
    
    ascendir_parser::IsaExecutor executor;
    executor.load(*module);
    executor.setVerbose(verboseMode);
    executor.setAsyncMode(!syncMode);
    
    if (dumpSequenceMode) {
      executor.getSequence().dump();
      llvm::outs() << "\n";
    }
    
    if (verboseMode) {
      llvm::outs() << "Starting simulation (mode: " << (syncMode ? "synchronous" : "asynchronous") << ")...\n\n";
      
      while (executor.tick()) {
      }
      
      llvm::outs() << "\nSimulation completed.\n";
      llvm::outs() << "Final PC: " << executor.getCurrentPC() << "\n";
      llvm::outs() << "Total cycles: " << executor.getCurrentCycle() << "\n";
    } else {
      executor.run();
      llvm::outs() << "Simulation completed.\n";
      llvm::outs() << "Total cycles: " << executor.getCurrentCycle() << "\n";
    }
  } else {
    parser.printModule(*module);
  }

  return 0;
}
