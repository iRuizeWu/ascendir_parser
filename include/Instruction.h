#pragma once

#include "mlir/IR/Value.h"
#include "mlir/IR/Operation.h"
#include <cstdint>
#include <string>
#include <vector>

namespace ascendir_parser {

struct Instruction {
    uint64_t pc;
    mlir::Operation* mlirOp;
    
    enum class Kind {
        Normal,
        Jump,
        ConditionalJump,
        Return,
        ForInit,
        ForCondition,
        ForIncrement,
        IfCondition
    };
    
    Kind kind = Kind::Normal;
    
    uint64_t jumpTarget = 0;
    uint64_t fallthroughPC = 0;
    mlir::Value condition;
    
    std::string sourceLocation;
    
    mlir::Value forIV;
    mlir::Value forLB;
    mlir::Value forUB;
    mlir::Value forStep;
    std::vector<mlir::Value> iterArgs;
    std::vector<mlir::Value> iterArgInits;
    mlir::Operation* forOp = nullptr;
    
    Instruction() : pc(0), mlirOp(nullptr), kind(Kind::Normal) {}
    
    static Instruction createNormal(uint64_t pc, mlir::Operation* op) {
        Instruction inst;
        inst.pc = pc;
        inst.mlirOp = op;
        inst.kind = Kind::Normal;
        return inst;
    }
    
    static Instruction createJump(uint64_t pc, uint64_t target) {
        Instruction inst;
        inst.pc = pc;
        inst.mlirOp = nullptr;
        inst.kind = Kind::Jump;
        inst.jumpTarget = target;
        return inst;
    }
    
    static Instruction createConditionalJump(uint64_t pc, mlir::Value cond, 
                                             uint64_t truePC, uint64_t falsePC) {
        Instruction inst;
        inst.pc = pc;
        inst.mlirOp = nullptr;
        inst.kind = Kind::ConditionalJump;
        inst.condition = cond;
        inst.jumpTarget = truePC;
        inst.fallthroughPC = falsePC;
        return inst;
    }
    
    static Instruction createReturn(uint64_t pc) {
        Instruction inst;
        inst.pc = pc;
        inst.mlirOp = nullptr;
        inst.kind = Kind::Return;
        return inst;
    }
    
    static Instruction createForInit(uint64_t pc, mlir::Operation* forOp,
                                     mlir::Value iv, mlir::Value lb,
                                     mlir::Value ub, mlir::Value step,
                                     const std::vector<mlir::Value>& iterArgs,
                                     const std::vector<mlir::Value>& iterArgInits) {
        Instruction inst;
        inst.pc = pc;
        inst.mlirOp = nullptr;
        inst.kind = Kind::ForInit;
        inst.forOp = forOp;
        inst.forIV = iv;
        inst.forLB = lb;
        inst.forUB = ub;
        inst.forStep = step;
        inst.iterArgs = iterArgs;
        inst.iterArgInits = iterArgInits;
        return inst;
    }
    
    static Instruction createForCondition(uint64_t pc, mlir::Value iv, 
                                          mlir::Value ub, mlir::Value step,
                                          uint64_t truePC, uint64_t falsePC) {
        Instruction inst;
        inst.pc = pc;
        inst.mlirOp = nullptr;
        inst.kind = Kind::ForCondition;
        inst.forIV = iv;
        inst.forUB = ub;
        inst.forStep = step;
        inst.jumpTarget = truePC;
        inst.fallthroughPC = falsePC;
        return inst;
    }
    
    static Instruction createForIncrement(uint64_t pc, mlir::Value iv, mlir::Value step) {
        Instruction inst;
        inst.pc = pc;
        inst.mlirOp = nullptr;
        inst.kind = Kind::ForIncrement;
        inst.forIV = iv;
        inst.forStep = step;
        return inst;
    }
    
    static Instruction createIfCondition(uint64_t pc, mlir::Value cond,
                                         uint64_t truePC, uint64_t falsePC) {
        Instruction inst;
        inst.pc = pc;
        inst.mlirOp = nullptr;
        inst.kind = Kind::IfCondition;
        inst.condition = cond;
        inst.jumpTarget = truePC;
        inst.fallthroughPC = falsePC;
        return inst;
    }
};

}
