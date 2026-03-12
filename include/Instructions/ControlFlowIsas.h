#pragma once

#include "Isa.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "llvm/Support/raw_ostream.h"

namespace ascendir_parser {

class JumpIsa : public Isa {
public:
    JumpIsa() {
        kind = Kind::Jump;
        isaName = IsaName::Jump;
    }
    
    void execute(ExecutionContext& ctx) override {
        // 控制流由IsaExecutor根据IsaName::Jump和jumpTarget处理
    }
    
    std::string getDescription() const override {
        return "jump -> PC " + std::to_string(jumpTarget);
    }
};

class ConditionalJumpIsa : public Isa {
public:
    ConditionalJumpIsa() {
        kind = Kind::ConditionalJump;
        isaName = IsaName::ConditionalJump;
    }
    
    void execute(ExecutionContext& ctx) override {
        // 控制流由IsaExecutor根据IsaName::ConditionalJump、condition、jumpTarget、fallthroughPC处理
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
        isaName = IsaName::Return;
    }
    
    void execute(ExecutionContext& ctx) override {
        // 控制流由IsaExecutor根据IsaName::Return处理
    }
    
    std::string getDescription() const override {
        return "return";
    }
};

class ForInitIsa : public Isa {
public:
    ForInitIsa() {
        kind = Kind::ForInit;
        isaName = IsaName::ForInit;
    }
    
    void execute(ExecutionContext& ctx) override {
        auto lbValue = ctx.getIntValue(forLB);
        ctx.setIntValue(forIV, lbValue);
        
        for (size_t i = 0; i < iterArgInits.size() && i < iterArgs.size(); ++i) {
            if (ctx.hasIntValue(iterArgInits[i])) {
                ctx.setIntValue(iterArgs[i], ctx.getIntValue(iterArgInits[i]));
            } else if (ctx.hasFloatValue(iterArgInits[i])) {
                ctx.setFloatValue(iterArgs[i], ctx.getFloatValue(iterArgInits[i]));
            }
        }
    }
    
    std::string getDescription() const override {
        return "for.init";
    }
};

class ForConditionIsa : public Isa {
public:
    ForConditionIsa() {
        kind = Kind::ForCondition;
        isaName = IsaName::ForCondition;
    }
    
    void execute(ExecutionContext& ctx) override {
        // 控制流由IsaExecutor根据IsaName::ForCondition处理
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
        isaName = IsaName::ForIncrement;
    }
    
    void execute(ExecutionContext& ctx) override {
        auto ivValue = ctx.getIntValue(forIV);
        auto stepValue = ctx.getIntValue(forStep);
        llvm::APInt newIV = ivValue + stepValue;
        ctx.setIntValue(forIV, newIV);
    }
    
    std::string getDescription() const override {
        return "for.increment";
    }
};

class IfConditionIsa : public Isa {
public:
    IfConditionIsa() {
        kind = Kind::IfCondition;
        isaName = IsaName::IfCondition;
    }
    
    void execute(ExecutionContext& ctx) override {
        // 控制流由IsaExecutor根据IsaName::IfCondition处理
    }
    
    std::string getDescription() const override {
        return "if.condition -> true:" + std::to_string(jumpTarget) + 
               " false:" + std::to_string(fallthroughPC);
    }
};

}
