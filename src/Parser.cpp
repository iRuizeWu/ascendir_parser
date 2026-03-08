#include "Parser.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "mlir/IR/BuiltinDialect.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/FileUtilities.h"
#include "llvm/ADT/StringRef.h"

namespace ascendir_parser {

class Parser::Impl {
public:
  mlir::MLIRContext context;
  
  Impl() {
    context.allowUnregisteredDialects(true);

    mlir::DialectRegistry registry;
    registry.insert<mlir::BuiltinDialect>();
    registry.insert<mlir::func::FuncDialect>();
    registry.insert<mlir::arith::ArithDialect>();
    registry.insert<mlir::memref::MemRefDialect>();
    registry.insert<mlir::LLVM::LLVMDialect>();
    registry.insert<mlir::cf::ControlFlowDialect>();
    registry.insert<mlir::scf::SCFDialect>();
    context.appendDialectRegistry(registry);
    context.loadAllAvailableDialects();
  }
};

Parser::Parser() : impl(std::make_unique<Impl>()) {}
Parser::~Parser() = default;

mlir::OwningOpRef<mlir::ModuleOp> Parser::parseFile(const std::string &filename) {
  llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> fileOrErr = 
      llvm::MemoryBuffer::getFileOrSTDIN(filename);
  if (std::error_code ec = fileOrErr.getError()) {
    llvm::errs() << "Error reading file: " << ec.message() << "\n";
    return nullptr;
  }

  llvm::SourceMgr sourceMgr;
  sourceMgr.AddNewSourceBuffer(std::move(*fileOrErr), llvm::SMLoc());

  auto module = mlir::parseSourceFile<mlir::ModuleOp>(sourceMgr, &impl->context);
  if (!module) {
    llvm::errs() << "Error parsing MLIR file\n";
    return nullptr;
  }

  return module;
}

void Parser::printModule(mlir::ModuleOp module) {
  module->print(llvm::outs());
}

void Parser::printOps(mlir::ModuleOp module, bool detailedFormat) {
  module.walk([&](mlir::Operation *op) {
    std::string opName = op->getName().getStringRef().str();
    std::string dialectNs = op->getName().getDialectNamespace().str();
    
    bool isUnregistered = (dialectNs.find("intr") != std::string::npos) || 
                          (opName.find("hivm.intr") != std::string::npos);
    
    if (detailedFormat) {
      if (isUnregistered) {
        llvm::outs() << "=== Unregistered Op: " << opName << " ===\n";
      } else {
        llvm::outs() << "=== Op: " << opName << " ===\n";
      }
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
    } else {
      std::string irStr;
      llvm::raw_string_ostream os(irStr);
      op->print(os);
      os.flush();
      llvm::outs() << irStr << "\n";
    }
  });
}

std::vector<FunctionInfo> Parser::extractFunctions(mlir::ModuleOp module) {
  std::vector<FunctionInfo> functions;
  
  module.walk([&](mlir::func::FuncOp funcOp) {
    FunctionInfo info;
    info.name = funcOp.getName().str();
    
    std::string irStr;
    llvm::raw_string_ostream os(irStr);
    funcOp.print(os);
    os.flush();
    info.irSource = irStr;
    
    functions.push_back(std::move(info));
  });
  
  return functions;
}

}
