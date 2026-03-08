#include "InstructionExecutor.h"
#include "InstructionRegistry.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/Support/raw_ostream.h"

namespace ascendir_parser {

void SequenceBuilder::buildFromMLIR(mlir::ModuleOp module) {
    for (auto& op : module.getBody()->getOperations()) {
        if (auto funcOp = llvm::dyn_cast<mlir::func::FuncOp>(op)) {
            flattenFuncOp(funcOp);
        }
    }
}

void SequenceBuilder::flattenFuncOp(mlir::Operation* funcOp) {
    if (auto func = llvm::dyn_cast<mlir::func::FuncOp>(funcOp)) {
        if (!func.getBody().empty()) {
            flattenRegion(func.getBody());
        }
    }
}

void SequenceBuilder::flattenRegion(mlir::Region& region) {
    for (auto& block : region.getBlocks()) {
        flattenBlock(block);
    }
}

void SequenceBuilder::flattenBlock(mlir::Block& block) {
    for (auto& op : block.getOperations()) {
        if (auto forOp = llvm::dyn_cast<mlir::scf::ForOp>(&op)) {
            flattenSCFFor(forOp);
        } else if (auto ifOp = llvm::dyn_cast<mlir::scf::IfOp>(&op)) {
            flattenSCFIf(ifOp);
        } else if (auto yieldOp = llvm::dyn_cast<mlir::scf::YieldOp>(&op)) {
            flattenSCFYield(yieldOp);
        } else {
            addNormalOp(&op);
        }
    }
}

void SequenceBuilder::flattenOperationsInBlock(mlir::Block& block, 
                                               const std::vector<mlir::Value>& iterArgs) {
    for (auto& op : block.getOperations()) {
        if (auto forOp = llvm::dyn_cast<mlir::scf::ForOp>(&op)) {
            flattenSCFFor(forOp);
        } else if (auto ifOp = llvm::dyn_cast<mlir::scf::IfOp>(&op)) {
            flattenSCFIfNested(ifOp, iterArgs);
        } else if (auto yieldOp = llvm::dyn_cast<mlir::scf::YieldOp>(&op)) {
            flattenSCFYield(yieldOp);
        } else {
            addNormalOp(&op);
        }
    }
}

void SequenceBuilder::flattenSCFFor(mlir::Operation* forOp) {
    auto op = llvm::cast<mlir::scf::ForOp>(forOp);
    
    mlir::Value iv = op.getInductionVar();
    mlir::Value lbValue = op.getLowerBound();
    mlir::Value ubValue = op.getUpperBound();
    mlir::Value stepValue = op.getStep();
    
    auto initArgs = op.getInitArgs();
    std::vector<mlir::Value> iterArgs;
    std::vector<mlir::Value> iterArgInits;
    
    for (auto arg : initArgs) {
        iterArgInits.push_back(arg);
    }
    
    for (auto arg : op.getRegionIterArgs()) {
        iterArgs.push_back(arg);
    }
    
    uint64_t initPC = currentPC();
    Instruction initInst = Instruction::createForInit(
        allocatePC(), forOp, iv, lbValue, ubValue, stepValue, iterArgs, iterArgInits);
    sequence.addInstruction(initInst);
    
    uint64_t condPC = currentPC();
    Instruction condInst = Instruction::createForCondition(
        allocatePC(), iv, ubValue, stepValue, 0, 0);
    sequence.addInstruction(condInst);
    
    uint64_t bodyStartPC = currentPC();
    
    if (!op.getBody()->empty()) {
        for (auto& bodyOp : op.getBody()->getOperations()) {
            if (auto yieldOp = llvm::dyn_cast<mlir::scf::YieldOp>(&bodyOp)) {
                for (size_t i = 0; i < yieldOp.getNumOperands() && i < iterArgs.size(); ++i) {
                    mlir::Value yieldVal = yieldOp.getOperand(i);
                    mlir::Value iterArg = iterArgs[i];
                    Instruction assignInst = Instruction::createNormal(allocatePC(), nullptr);
                    assignInst.sourceLocation = "scf.for.yield_assign";
                    assignInst.condition = yieldVal;
                    assignInst.forIV = iterArg;
                    sequence.addInstruction(assignInst);
                }
            } else if (auto nestedIfOp = llvm::dyn_cast<mlir::scf::IfOp>(&bodyOp)) {
                flattenSCFIfNested(nestedIfOp, iterArgs);
            } else if (auto nestedForOp = llvm::dyn_cast<mlir::scf::ForOp>(&bodyOp)) {
                flattenSCFFor(nestedForOp);
            } else {
                addNormalOp(&bodyOp);
            }
        }
    }
    
    uint64_t incrementPC = currentPC();
    Instruction incInst = Instruction::createForIncrement(allocatePC(), iv, stepValue);
    sequence.addInstruction(incInst);
    
    uint64_t jumpBackPC = currentPC();
    Instruction jumpBackInst = Instruction::createJump(allocatePC(), condPC);
    sequence.addInstruction(jumpBackInst);
    
    uint64_t afterLoopPC = currentPC();
    
    sequence.updateConditionalJump(condPC, bodyStartPC, afterLoopPC);
}

void SequenceBuilder::flattenSCFIf(mlir::Operation* ifOp) {
    flattenSCFIfNested(llvm::cast<mlir::scf::IfOp>(ifOp), {});
}

void SequenceBuilder::flattenSCFIfNested(mlir::scf::IfOp op, 
                                          const std::vector<mlir::Value>& parentIterArgs) {
    mlir::Value condValue = op.getCondition();
    
    uint64_t condPC = currentPC();
    Instruction condInst = Instruction::createIfCondition(
        allocatePC(), condValue, 0, 0);
    sequence.addInstruction(condInst);
    
    uint64_t thenStartPC = currentPC();
    
    std::vector<mlir::Value> thenYieldValues;
    if (!op.getThenRegion().empty()) {
        for (auto& thenOp : op.getThenRegion().front().getOperations()) {
            if (auto yieldOp = llvm::dyn_cast<mlir::scf::YieldOp>(&thenOp)) {
                for (auto operand : yieldOp.getOperands()) {
                    thenYieldValues.push_back(operand);
                }
            } else if (auto nestedIfOp = llvm::dyn_cast<mlir::scf::IfOp>(&thenOp)) {
                flattenSCFIfNested(nestedIfOp, parentIterArgs);
            } else if (auto nestedForOp = llvm::dyn_cast<mlir::scf::ForOp>(&thenOp)) {
                flattenSCFFor(nestedForOp);
            } else {
                addNormalOp(&thenOp);
            }
        }
    }
    
    for (size_t i = 0; i < thenYieldValues.size() && i < op.getNumResults(); ++i) {
        Instruction assignInst = Instruction::createNormal(allocatePC(), nullptr);
        assignInst.sourceLocation = "scf.if.then_yield";
        assignInst.condition = thenYieldValues[i];
        assignInst.forIV = op.getResult(i);
        sequence.addInstruction(assignInst);
    }
    
    uint64_t thenJumpPC = 0;
    bool hasElse = !op.getElseRegion().empty();
    if (hasElse) {
        thenJumpPC = currentPC();
        Instruction thenJumpInst = Instruction::createJump(allocatePC(), 0);
        sequence.addInstruction(thenJumpInst);
    }
    
    uint64_t elseStartPC = currentPC();
    
    std::vector<mlir::Value> elseYieldValues;
    if (hasElse) {
        for (auto& elseOp : op.getElseRegion().front().getOperations()) {
            if (auto yieldOp = llvm::dyn_cast<mlir::scf::YieldOp>(&elseOp)) {
                for (auto operand : yieldOp.getOperands()) {
                    elseYieldValues.push_back(operand);
                }
            } else if (auto nestedIfOp = llvm::dyn_cast<mlir::scf::IfOp>(&elseOp)) {
                flattenSCFIfNested(nestedIfOp, parentIterArgs);
            } else if (auto nestedForOp = llvm::dyn_cast<mlir::scf::ForOp>(&elseOp)) {
                flattenSCFFor(nestedForOp);
            } else {
                addNormalOp(&elseOp);
            }
        }
        
        for (size_t i = 0; i < elseYieldValues.size() && i < op.getNumResults(); ++i) {
            Instruction assignInst = Instruction::createNormal(allocatePC(), nullptr);
            assignInst.sourceLocation = "scf.if.else_yield";
            assignInst.condition = elseYieldValues[i];
            assignInst.forIV = op.getResult(i);
            sequence.addInstruction(assignInst);
        }
    }
    
    uint64_t afterIfPC = currentPC();
    
    if (hasElse) {
        sequence.updateConditionalJump(condPC, thenStartPC, elseStartPC);
        sequence.updateJumpTarget(thenJumpPC, afterIfPC);
    } else {
        sequence.updateConditionalJump(condPC, thenStartPC, afterIfPC);
    }
}

void SequenceBuilder::flattenSCFYield(mlir::Operation* yieldOp) {
}

void SequenceBuilder::addNormalOp(mlir::Operation* op) {
    uint64_t pc = allocatePC();
    Instruction inst = Instruction::createNormal(pc, op);
    sequence.addInstruction(inst);
}

void SequenceBuilder::addJump(uint64_t targetPC) {
    uint64_t pc = allocatePC();
    Instruction inst = Instruction::createJump(pc, targetPC);
    sequence.addInstruction(inst);
}

void SequenceBuilder::addConditionalJump(mlir::Value condition, uint64_t truePC, uint64_t falsePC) {
    uint64_t pc = allocatePC();
    Instruction inst = Instruction::createConditionalJump(pc, condition, truePC, falsePC);
    sequence.addInstruction(inst);
}

void SequenceBuilder::addReturn() {
    uint64_t pc = allocatePC();
    Instruction inst = Instruction::createReturn(pc);
    sequence.addInstruction(inst);
}

void InstructionExecutor::load(mlir::ModuleOp module) {
    SequenceBuilder builder;
    builder.buildFromMLIR(module);
    sequence = builder.getSequence();
    
    ctx.setPC(0);
}

bool InstructionExecutor::executeNext() {
    if (ctx.isHalted()) {
        return false;
    }
    
    Instruction* inst = sequence.getInstructionByPC(ctx.getPC());
    if (!inst) {
        ctx.halt();
        return false;
    }
    
    executeInstruction(inst);
    return true;
}

void InstructionExecutor::run() {
    while (!ctx.isHalted()) {
        executeNext();
    }
}

void InstructionExecutor::executeInstruction(Instruction* inst) {
    uint64_t latency = 1;
    
    switch (inst->kind) {
        case Instruction::Kind::Normal:
            latency = executeNormalInstruction(inst);
            break;
        case Instruction::Kind::Jump:
            executeJumpInstruction(inst);
            break;
        case Instruction::Kind::ConditionalJump:
        case Instruction::Kind::IfCondition:
            executeConditionalJumpInstruction(inst);
            break;
        case Instruction::Kind::ForInit:
            executeForInitInstruction(inst);
            break;
        case Instruction::Kind::ForCondition:
            executeForConditionInstruction(inst);
            break;
        case Instruction::Kind::ForIncrement:
            executeForIncrementInstruction(inst);
            break;
        case Instruction::Kind::Return:
            executeReturnInstruction();
            break;
    }
    
    ctx.advanceCycle(latency);
}

uint64_t InstructionExecutor::executeNormalInstruction(Instruction* inst) {
    uint64_t latency = 1;
    if (inst->mlirOp) {
        latency = executeOp(inst->mlirOp);
    } else if (!inst->sourceLocation.empty()) {
        handleYieldAssignment(inst);
    }
    advancePC();
    return latency;
}

void InstructionExecutor::executeJumpInstruction(Instruction* inst) {
    ctx.setPC(inst->jumpTarget);
}

void InstructionExecutor::executeConditionalJumpInstruction(Instruction* inst) {
    llvm::APInt condVal = ctx.getIntValue(inst->condition);
    bool cond = (condVal != 0);
    ctx.setPC(cond ? inst->jumpTarget : inst->fallthroughPC);
}

void InstructionExecutor::executeForInitInstruction(Instruction* inst) {
    llvm::APInt lbVal = ctx.getIntValue(inst->forLB);
    ctx.setIntValue(inst->forIV, lbVal);
    
    for (size_t i = 0; i < inst->iterArgs.size() && i < inst->iterArgInits.size(); ++i) {
        copyValue(inst->iterArgs[i], inst->iterArgInits[i]);
    }
    
    advancePC();
}

void InstructionExecutor::executeForConditionInstruction(Instruction* inst) {
    llvm::APInt ivVal = ctx.getIntValue(inst->forIV);
    llvm::APInt ubVal = ctx.getIntValue(inst->forUB);
    bool cond = ivVal.slt(ubVal);
    ctx.setPC(cond ? inst->jumpTarget : inst->fallthroughPC);
}

void InstructionExecutor::executeForIncrementInstruction(Instruction* inst) {
    llvm::APInt ivVal = ctx.getIntValue(inst->forIV);
    llvm::APInt stepVal = ctx.getIntValue(inst->forStep);
    ctx.setIntValue(inst->forIV, ivVal + stepVal);
    advancePC();
}

void InstructionExecutor::executeReturnInstruction() {
    if (!ctx.hasCallFrames()) {
        ctx.halt();
    } else {
        ctx.setPC(ctx.popCallFrame());
    }
}

void InstructionExecutor::handleYieldAssignment(Instruction* inst) {
    if (inst->sourceLocation.find("yield") == std::string::npos) {
        return;
    }
    if (!inst->condition || !inst->forIV) {
        return;
    }
    copyValue(inst->forIV, inst->condition);
}

void InstructionExecutor::copyValue(mlir::Value dest, mlir::Value src) {
    auto type = dest.getType();
    
    if (type.isIndex() || type.isSignlessInteger() || type.isSignedInteger()) {
        ctx.setIntValue(dest, ctx.getIntValue(src));
    } else if (type.isF32() || type.isF64() || type.isBF16() || type.isF16() || 
               type.isFloat8E5M2() || type.isFloat8E4M3FN()) {
        ctx.setFloatValue(dest, ctx.getFloatValue(src));
    }
}

void InstructionExecutor::advancePC() {
    ctx.setPC(sequence.getNextPC(ctx.getPC()));
}

uint64_t InstructionExecutor::executeOp(mlir::Operation* op) {
    if (!op) {
        return 1;
    }
    
    std::string opName = op->getName().getStringRef().str();
    
    auto& registry = getGlobalRegistry();
    if (registry.hasHandler(opName)) {
        auto* info = registry.getInstructionInfo(opName);
        if (info && info->handler) {
            info->handler(op, ctx);
            return info->latency;
        }
    } else {
        llvm::errs() << "Unknown operation: " << opName << "\n";
        ctx.halt();
    }
    return 1;
}

}
