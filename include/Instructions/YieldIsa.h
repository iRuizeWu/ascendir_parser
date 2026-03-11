#pragma once

#include "Isa.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "llvm/Support/raw_ostream.h"

namespace ascendir_parser {

class YieldAssignIsa : public Isa {
public:
    YieldAssignIsa() {
        kind = Kind::Normal;
    }
    
    IsaExecuteResult execute(ExecutionContext& ctx) override {
        if (condition && forIV) {
            if (ctx.hasIntValue(condition)) {
                ctx.setIntValue(forIV, ctx.getIntValue(condition));
            } else if (ctx.hasFloatValue(condition)) {
                ctx.setFloatValue(forIV, ctx.getFloatValue(condition));
            }
        }
        return IsaExecuteResult::continueExecution();
    }
    
    std::string getDescription() const override {
        return "yield.assign";
    }
};

}
