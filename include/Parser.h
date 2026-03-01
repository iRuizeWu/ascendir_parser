#pragma once

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/OwningOpRef.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>
#include <vector>
#include <string>

namespace ascendir_parser {

struct FunctionInfo {
  std::string name;
  std::string irSource;
};

class Parser {
public:
  Parser();
  ~Parser();

  mlir::OwningOpRef<mlir::ModuleOp> parseFile(const std::string &filename);
  void printModule(mlir::ModuleOp module);
  void printOps(mlir::ModuleOp module, bool detailedFormat = false);
  
  std::vector<FunctionInfo> extractFunctions(mlir::ModuleOp module);

private:
  class Impl;
  std::unique_ptr<Impl> impl;
};

void analyzeUnregisteredOps(mlir::ModuleOp module);

}