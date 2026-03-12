#pragma once

#include "Isa.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "llvm/Support/raw_ostream.h"

namespace ascendir_parser {

class FuncCallIsa : public Isa {
public:
    FuncCallIsa() {
        isaName = IsaName::FuncCall;
    }
    
    uint64_t getLatency() const override { return 5; }
    
    void execute(ExecutionContext& ctx) override {
        // 控制流由IsaExecutor根据IsaName::FuncCall处理
        llvm::errs() << "func.call not yet implemented\n";
    }
    
    std::string getDescription() const override {
        return "func.call";
    }
};

class FuncReturnIsa : public Isa {
public:
    FuncReturnIsa() {
        isaName = IsaName::FuncReturn;
    }
    
    uint64_t getLatency() const override { return 1; }
    
    void execute(ExecutionContext& ctx) override {
        // 控制流由IsaExecutor根据IsaName::FuncReturn处理
    }
    
    std::string getDescription() const override {
        return "func.return";
    }
};

inline void registerFuncIsas() {
    REGISTER_ISA("func.call", FuncCallIsa, IsaName::FuncCall);
    REGISTER_ISA("func.return", FuncReturnIsa, IsaName::FuncReturn);
}

}