#pragma once

#include "InstructionSequence.h"
#include "ExecutionContext.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"

namespace ascendir_parser {

class SequenceBuilder {
    InstructionSequence sequence;
    uint64_t nextPC = 0;
    
public:
    void buildFromMLIR(mlir::ModuleOp module);
    InstructionSequence& getSequence() { return sequence; }
    
private:
    uint64_t allocatePC() { return nextPC++; }
    uint64_t currentPC() const { return nextPC; }
    
    void addNormalOp(mlir::Operation* op);
    void addJump(uint64_t targetPC);
    void addConditionalJump(mlir::Value condition, uint64_t truePC, uint64_t falsePC);
    void addReturn();
    
    void flattenRegion(mlir::Region& region);
    void flattenBlock(mlir::Block& block);
    void flattenFuncOp(mlir::Operation* funcOp);
    
    void flattenOperationsInBlock(mlir::Block& block, 
                                   const std::vector<mlir::Value>& iterArgs);
    
    void flattenSCFFor(mlir::Operation* forOp);
    void flattenSCFIf(mlir::Operation* ifOp);
    void flattenSCFIfNested(mlir::scf::IfOp op, 
                             const std::vector<mlir::Value>& parentIterArgs);
    void flattenSCFYield(mlir::Operation* yieldOp);
};

class InstructionExecutor {
    InstructionSequence sequence;
    ExecutionContext ctx;
    
public:
    void load(mlir::ModuleOp module);
    bool executeNext();
    void run();
    
    uint64_t getCurrentPC() const { return ctx.getPC(); }
    bool isHalted() const { return ctx.isHalted(); }
    
    ExecutionContext& getContext() { return ctx; }
    InstructionSequence& getSequence() { return sequence; }
    
private:
    void executeInstruction(Instruction* inst);
    uint64_t executeOp(mlir::Operation* op);
    
    uint64_t executeNormalInstruction(Instruction* inst);
    void executeJumpInstruction(Instruction* inst);
    void executeConditionalJumpInstruction(Instruction* inst);
    void executeForInitInstruction(Instruction* inst);
    void executeForConditionInstruction(Instruction* inst);
    void executeForIncrementInstruction(Instruction* inst);
    void executeReturnInstruction();
    
    void handleYieldAssignment(Instruction* inst);
    void copyValue(mlir::Value dest, mlir::Value src);
    void advancePC();
};

}
