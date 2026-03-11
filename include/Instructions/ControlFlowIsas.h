#pragma once

#include "Isa.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "llvm/Support/raw_ostream.h"

namespace ascendir_parser {

class JumpIsa : public Isa {
public:
    JumpIsa() {
        kind = Kind::Jump;
    }
    
    IsaExecuteResult execute(ExecutionContext& ctx) override {
        return IsaExecuteResult::jumpTo(jumpTarget);
    }
    
    std::string getDescription() const override {
        return "jump -> PC " + std::to_string(jumpTarget);
    }
};

class ConditionalJumpIsa : public Isa {
public:
    ConditionalJumpIsa() {
        kind = Kind::ConditionalJump;
    }
    
    IsaExecuteResult execute(ExecutionContext& ctx) override {
        if (condition) {
            auto condValue = ctx.getIntValue(condition);
            bool cond = condValue != 0;
            
            if (cond) {
                return IsaExecuteResult::jumpTo(jumpTarget);
            } else {
                return IsaExecuteResult::jumpTo(fallthroughPC);
            }
        }
        return IsaExecuteResult::continueExecution();
    }
    
    std::string getDescription() const override {
        return "conditional_jump -> true:" + std::to_string(jumpTarget) + 
               " false:" + std::to_string(fallthroughPC);
    }
};

class ReturnIsa : public Isa {
public:
    ReturnIsa() {
        kind = Kind::Return;
    }
    
    IsaExecuteResult execute(ExecutionContext& ctx) override {
        if (!ctx.hasCallFrames()) {
            return IsaExecuteResult::halt();
        } else {
            return IsaExecuteResult::ret();
        }
    }
    
    std::string getDescription() const override {
        return "return";
    }
};

class ForInitIsa : public Isa {
public:
    ForInitIsa() {
        kind = Kind::ForInit;
    }
    
    IsaExecuteResult execute(ExecutionContext& ctx) override {
        auto lbValue = ctx.getIntValue(forLB);
        ctx.setIntValue(forIV, lbValue);
        
        for (size_t i = 0; i < iterArgInits.size() && i < iterArgs.size(); ++i) {
            if (ctx.hasIntValue(iterArgInits[i])) {
                ctx.setIntValue(iterArgs[i], ctx.getIntValue(iterArgInits[i]));
            } else if (ctx.hasFloatValue(iterArgInits[i])) {
                ctx.setFloatValue(iterArgs[i], ctx.getFloatValue(iterArgInits[i]));
            }
        }
        return IsaExecuteResult::continueExecution();
    }
    
    std::string getDescription() const override {
        return "for.init";
    }
};

class ForConditionIsa : public Isa {
public:
    ForConditionIsa() {
        kind = Kind::ForCondition;
    }
    
    IsaExecuteResult execute(ExecutionContext& ctx) override {
        auto ivValue = ctx.getIntValue(forIV);
        auto ubValue = ctx.getIntValue(forUB);
        auto stepValue = ctx.getIntValue(forStep);
        
        bool continueLoop = false;
        if (stepValue.sgt(llvm::APInt(stepValue.getBitWidth(), 0))) {
            continueLoop = ivValue.slt(ubValue);
        } else {
            continueLoop = ivValue.sgt(ubValue);
        }
        
        if (continueLoop) {
            return IsaExecuteResult::jumpTo(jumpTarget);
        } else {
            return IsaExecuteResult::jumpTo(fallthroughPC);
        }
    }
    
    std::string getDescription() const override {
        return "for.condition -> true:" + std::to_string(jumpTarget) + 
               " false:" + std::to_string(fallthroughPC);
    }
};

class ForIncrementIsa : public Isa {
public:
    ForIncrementIsa() {
        kind = Kind::ForIncrement;
    }
    
    IsaExecuteResult execute(ExecutionContext& ctx) override {
        auto ivValue = ctx.getIntValue(forIV);
        auto stepValue = ctx.getIntValue(forStep);
        llvm::APInt newIV = ivValue + stepValue;
        ctx.setIntValue(forIV, newIV);
        return IsaExecuteResult::continueExecution();
    }
    
    std::string getDescription() const override {
        return "for.increment";
    }
};

class IfConditionIsa : public Isa {
public:
    IfConditionIsa() {
        kind = Kind::IfCondition;
    }
    
    IsaExecuteResult execute(ExecutionContext& ctx) override {
        if (condition) {
            auto condValue = ctx.getIntValue(condition);
            bool cond = condValue != 0;
            
            if (cond) {
                return IsaExecuteResult::jumpTo(jumpTarget);
            } else {
                return IsaExecuteResult::jumpTo(fallthroughPC);
            }
        }
        return IsaExecuteResult::continueExecution();
    }
    
    std::string getDescription() const override {
        return "if.condition -> true:" + std::to_string(jumpTarget) + 
               " false:" + std::to_string(fallthroughPC);
    }
};

}
