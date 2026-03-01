#include "Parser.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Attributes.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>

namespace ascendir_parser {

void analyzeUnregisteredOps(mlir::ModuleOp module) {
  module.walk([&](mlir::Operation *op) {
    std::string opName = op->getName().getStringRef().str();
    std::string dialectNs = op->getName().getDialectNamespace().str();
    
    bool isUnregistered = (dialectNs.find("intr") != std::string::npos) || 
                          (opName.find("hivm.intr") != std::string::npos);
    
    if (isUnregistered) {
      llvm::outs() << "=== Unregistered Op: " << opName << " ===\n";
      
      llvm::outs() << "  OpName (string): " << op->getName().getStringRef() << "\n";
      llvm::outs() << "  DialectName: " << op->getName().getDialectNamespace() << "\n";
      llvm::outs() << "  OperationName: " << op->getName().stripDialect() << "\n";
      
      llvm::outs() << "  Attributes:\n";
      llvm::outs() << "    Raw attr dict: " << op->getAttrDictionary() << "\n";
      for (mlir::NamedAttribute attr : op->getAttrs()) {
        llvm::outs() << "    " << attr.getName() << " = ";
        attr.getValue().print(llvm::outs());
        llvm::outs() << "\n";
      }
      
      llvm::outs() << "  Operands (" << op->getNumOperands() << "):\n";
      for (unsigned i = 0; i < op->getNumOperands(); ++i) {
        llvm::outs() << "    [" << i << "]: " << op->getOperand(i) 
                     << " (type: " << op->getOperand(i).getType() << ")\n";
      }
      
      llvm::outs() << "  Results (" << op->getNumResults() << "):\n";
      for (unsigned i = 0; i < op->getNumResults(); ++i) {
        llvm::outs() << "    [" << i << "]: type = " << op->getResult(i).getType() << "\n";
      }
      
      std::string irStr;
      llvm::raw_string_ostream os(irStr);
      op->print(os);
      os.flush();
      llvm::outs() << "  Full IR: " << irStr << "\n";
      
      llvm::outs() << "\n";
    }
  });
}

}