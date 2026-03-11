#pragma once

#include "Isa.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "llvm/Support/raw_ostream.h"

namespace ascendir_parser {

class FuncCallIsa : public Isa {
public:
    uint64_t getLatency() const override { return 5; }
    
    IsaExecuteResult execute(ExecutionContext& ctx) override {
        llvm::errs() << "func.call not yet implemented\n";
        return IsaExecuteResult::halt();
    }
    
    std::string getDescription() const override {
        return "func.call";
    }
};

class FuncReturnIsa : public Isa {
public:
    uint64_t getLatency() const override { return 1; }
    
    IsaExecuteResult execute(ExecutionContext& ctx) override {
        if (!ctx.hasCallFrames()) {
            return IsaExecuteResult::halt();
        } else {
            return IsaExecuteResult::ret();
        }
    }
    
    std::string getDescription() const override {
        return "func.return";
    }
};

inline void registerFuncIsas() {
    REGISTER_ISA("func.call", FuncCallIsa);
    REGISTER_ISA("func.return", FuncReturnIsa);
}

}
