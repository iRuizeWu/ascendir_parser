#include "Parser.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Parser/Parser.h"
#include "mlir/IR/BuiltinDialect.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/FileUtilities.h"
#include "llvm/ADT/StringRef.h"

#ifdef USE_BISHENGIR
#include "bishengir/Dialect/HIVM/IR/HIVM.h"
#include "bishengir/Dialect/HFusion/IR/HFusion.h"
#include "bishengir/Dialect/HACC/IR/HACC.h"
#include "bishengir/Dialect/Annotation/IR/Annotation.h"
#include "bishengir/Dialect/MemRefExt/IR/MemRefExt.h"
#endif

namespace ascendir_parser {

class Parser::Impl {
public:
  mlir::MLIRContext context;
  
  Impl() {
    mlir::DialectRegistry registry;
    registry.insert<mlir::BuiltinDialect>();
    registry.insert<mlir::func::FuncDialect>();
    registry.insert<mlir::arith::ArithDialect>();
    registry.insert<mlir::memref::MemRefDialect>();
#ifdef USE_BISHENGIR
    registry.insert<mlir::hivm::HIVMDialect>();
    registry.insert<mlir::hfusion::HFusionDialect>();
    registry.insert<mlir::hacc::HACCDialect>();
    registry.insert<mlir::annotation::AnnotationDialect>();
    registry.insert<bishengir::memref_ext::MemRefExtDialect>();
#endif
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
