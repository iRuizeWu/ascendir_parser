#include "IsaSequence.h"
#include "IsaRegistry.h"
#include "Instructions/ArithIsas.h"
#include "Instructions/FuncIsas.h"
#include "Instructions/HivmIsas.h"
#include "Instructions/ControlFlowIsas.h"
#include "Instructions/YieldIsa.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/Support/raw_ostream.h"

namespace ascendir_parser {

void IsaSequence::addInstruction(IsaPtr inst) {
    if (!inst) return;
    
    uint64_t pc = inst->getPC();
    pcToIndex[pc] = instructions.size();
    instructions.push_back(std::move(inst));
}

Isa* IsaSequence::getInstructionByPC(uint64_t pc) {
    auto it = pcToIndex.find(pc);
    if (it == pcToIndex.end()) {
        return nullptr;
    }
    return instructions[it->second].get();
}

const Isa* IsaSequence::getInstructionByPCConst(uint64_t pc) const {
    auto it = pcToIndex.find(pc);
    if (it == pcToIndex.end()) {
        return nullptr;
    }
    return instructions[it->second].get();
}

uint64_t IsaSequence::getNextPC(uint64_t currentPC) {
    return currentPC + 1;
}

uint64_t IsaSequence::allocatePC() const {
    return instructions.size();
}

void IsaSequence::setLabel(const std::string& label, uint64_t pc) {
    labels[label] = pc;
}

uint64_t IsaSequence::getLabelPC(const std::string& label) {
    auto it = labels.find(label);
    if (it == labels.end()) {
        return 0;
    }
    return it->second;
}

bool IsaSequence::isValidPC(uint64_t pc) const {
    return pcToIndex.find(pc) != pcToIndex.end();
}

void IsaSequence::updateConditionalJump(uint64_t pc, uint64_t truePC, uint64_t falsePC) {
    Isa* inst = getInstructionByPC(pc);
    if (inst && (inst->getKind() == Isa::Kind::ConditionalJump || 
                 inst->getKind() == Isa::Kind::ForCondition ||
                 inst->getKind() == Isa::Kind::IfCondition)) {
        inst->setJumpTarget(truePC);
        inst->setFallthroughPC(falsePC);
    }
}

void IsaSequence::updateJumpTarget(uint64_t pc, uint64_t targetPC) {
    Isa* inst = getInstructionByPC(pc);
    if (inst && inst->getKind() == Isa::Kind::Jump) {
        inst->setJumpTarget(targetPC);
    }
}

std::string IsaSequence::getInstructionDescription(uint64_t pc) const {
    const Isa* inst = getInstructionByPCConst(pc);
    if (!inst) {
        return "Invalid PC";
    }
    return inst->getDescription();
}

void IsaSequence::dump() {
    llvm::errs() << "=== ISA Sequence Dump ===\n";
    for (size_t i = 0; i < instructions.size(); ++i) {
        const Isa* inst = instructions[i].get();
        llvm::errs() << "PC[" << inst->getPC() << "] "
                     << "IsaID[" << inst->getIsaID() << "] "
                     << inst->getDescription() << "\n";
    }
}

void IsaSequenceBuilder::buildFromMLIR(mlir::ModuleOp module) {
    for (auto& op : module.getBody()->getOperations()) {
        if (auto funcOp = llvm::dyn_cast<mlir::func::FuncOp>(op)) {
            flattenFuncOp(funcOp);
        }
    }
}

void IsaSequenceBuilder::flattenFuncOp(mlir::Operation* funcOp) {
    if (auto func = llvm::dyn_cast<mlir::func::FuncOp>(funcOp)) {
        if (!func.getBody().empty()) {
            flattenRegion(func.getBody());
        }
    }
}

void IsaSequenceBuilder::flattenRegion(mlir::Region& region) {
    for (auto& block : region.getBlocks()) {
        flattenBlock(block);
    }
}

void IsaSequenceBuilder::flattenBlock(mlir::Block& block) {
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

void IsaSequenceBuilder::flattenOperationsInBlock(mlir::Block& block, 
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

void IsaSequenceBuilder::flattenSCFFor(mlir::Operation* forOp) {
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
    
    auto initIsa = std::make_unique<ForInitIsa>();
    initIsa->setPC(allocatePC());
    initIsa->setForOp(forOp);
    initIsa->setForIV(iv);
    initIsa->setForLB(lbValue);
    initIsa->setForUB(ubValue);
    initIsa->setForStep(stepValue);
    initIsa->setIterArgs(iterArgs);
    initIsa->setIterArgInits(iterArgInits);
    sequence.addInstruction(std::move(initIsa));
    
    uint64_t condPC = currentPC();
    auto condIsa = std::make_unique<ForConditionIsa>();
    condIsa->setPC(allocatePC());
    condIsa->setForIV(iv);
    condIsa->setForUB(ubValue);
    condIsa->setForStep(stepValue);
    sequence.addInstruction(std::move(condIsa));
    
    uint64_t bodyStartPC = currentPC();
    
    if (!op.getBody()->empty()) {
        for (auto& bodyOp : op.getBody()->getOperations()) {
            if (auto yieldOp = llvm::dyn_cast<mlir::scf::YieldOp>(&bodyOp)) {
                for (size_t i = 0; i < yieldOp.getNumOperands() && i < iterArgs.size(); ++i) {
                    mlir::Value yieldVal = yieldOp.getOperand(i);
                    mlir::Value iterArg = iterArgs[i];
                    
                    auto assignIsa = std::make_unique<YieldAssignIsa>();
                    assignIsa->setPC(allocatePC());
                    assignIsa->setLocationInfo("scf.for.yield");
                    assignIsa->setCondition(yieldVal);
                    assignIsa->setForIV(iterArg);
                    sequence.addInstruction(std::move(assignIsa));
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
    
    auto incIsa = std::make_unique<ForIncrementIsa>();
    incIsa->setPC(allocatePC());
    incIsa->setForIV(iv);
    incIsa->setForStep(stepValue);
    sequence.addInstruction(std::move(incIsa));
    
    auto jumpBackIsa = std::make_unique<JumpIsa>();
    jumpBackIsa->setPC(allocatePC());
    jumpBackIsa->setJumpTarget(condPC);
    sequence.addInstruction(std::move(jumpBackIsa));
    
    uint64_t afterLoopPC = currentPC();
    
    sequence.updateConditionalJump(condPC, bodyStartPC, afterLoopPC);
}

void IsaSequenceBuilder::flattenSCFIf(mlir::Operation* ifOp) {
    flattenSCFIfNested(llvm::cast<mlir::scf::IfOp>(ifOp), {});
}

void IsaSequenceBuilder::flattenSCFIfNested(mlir::scf::IfOp op, 
                                              const std::vector<mlir::Value>& parentIterArgs) {
    mlir::Value condValue = op.getCondition();
    auto ifResults = op.getResults();
    
    uint64_t condPC = currentPC();
    auto condIsa = std::make_unique<IfConditionIsa>();
    condIsa->setPC(allocatePC());
    condIsa->setCondition(condValue);
    sequence.addInstruction(std::move(condIsa));
    
    uint64_t thenStartPC = currentPC();
    
    if (!op.getThenRegion().empty()) {
        for (auto& thenOp : op.getThenRegion().front().getOperations()) {
            if (auto yieldOp = llvm::dyn_cast<mlir::scf::YieldOp>(&thenOp)) {
                for (size_t i = 0; i < yieldOp.getNumOperands() && i < parentIterArgs.size(); ++i) {
                    mlir::Value yieldVal = yieldOp.getOperand(i);
                    mlir::Value iterArg = parentIterArgs[i];
                    
                    auto assignIsa = std::make_unique<YieldAssignIsa>();
                    assignIsa->setPC(allocatePC());
                    assignIsa->setLocationInfo("scf.if.then.yield");
                    assignIsa->setCondition(yieldVal);
                    assignIsa->setForIV(iterArg);
                    sequence.addInstruction(std::move(assignIsa));
                }
                
                if (parentIterArgs.empty() && yieldOp.getNumOperands() > 0) {
                    auto ifYieldIsa = std::make_unique<IfYieldIsa>();
                    ifYieldIsa->setPC(allocatePC());
                    ifYieldIsa->setLocationInfo("scf.if.then");
                    ifYieldIsa->setIfResults(yieldOp.getOperands());
                    sequence.addInstruction(std::move(ifYieldIsa));
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
    
    uint64_t elseStartPC = currentPC();
    bool hasElse = !op.getElseRegion().empty();
    if (hasElse) {
        for (auto& elseOp : op.getElseRegion().front().getOperations()) {
            if (auto yieldOp = llvm::dyn_cast<mlir::scf::YieldOp>(&elseOp)) {
                for (size_t i = 0; i < yieldOp.getNumOperands() && i < parentIterArgs.size(); ++i) {
                    mlir::Value yieldVal = yieldOp.getOperand(i);
                    mlir::Value iterArg = parentIterArgs[i];
                    
                    auto assignIsa = std::make_unique<YieldAssignIsa>();
                    assignIsa->setPC(allocatePC());
                    assignIsa->setLocationInfo("scf.if.else.yield");
                    assignIsa->setCondition(yieldVal);
                    assignIsa->setForIV(iterArg);
                    sequence.addInstruction(std::move(assignIsa));
                }
                
                if (parentIterArgs.empty() && yieldOp.getNumOperands() > 0) {
                    auto ifYieldIsa = std::make_unique<IfYieldIsa>();
                    ifYieldIsa->setPC(allocatePC());
                    ifYieldIsa->setLocationInfo("scf.if.else");
                    ifYieldIsa->setIfResults(yieldOp.getOperands());
                    sequence.addInstruction(std::move(ifYieldIsa));
                }
            } else if (auto nestedIfOp = llvm::dyn_cast<mlir::scf::IfOp>(&elseOp)) {
                flattenSCFIfNested(nestedIfOp, parentIterArgs);
            } else if (auto nestedForOp = llvm::dyn_cast<mlir::scf::ForOp>(&elseOp)) {
                flattenSCFFor(nestedForOp);
            } else {
                addNormalOp(&elseOp);
            }
        }
    }
    
    uint64_t jumpPC = 0;
    if (hasElse) {
        jumpPC = currentPC();
        auto thenJumpIsa = std::make_unique<JumpIsa>();
        thenJumpIsa->setPC(allocatePC());
        sequence.addInstruction(std::move(thenJumpIsa));
    }
    
    uint64_t afterIfPC = currentPC();
    
    if (hasElse && jumpPC > 0) {
        sequence.updateJumpTarget(jumpPC, afterIfPC);
    }
    
    uint64_t finalElsePC = hasElse ? elseStartPC : afterIfPC;
    sequence.updateConditionalJump(condPC, thenStartPC, finalElsePC);
}

void IsaSequenceBuilder::flattenSCFYield(mlir::Operation* yieldOp) {
}

void IsaSequenceBuilder::addNormalOp(mlir::Operation* op) {
    if (!op) return;
    
    std::string opName = op->getName().getStringRef().str();
    
    IsaPtr isa = IsaRegistry::getGlobalRegistry().createIsa(opName);
    if (!isa) {
        isa = std::make_unique<ArithConstantIsa>();
    }
    
    isa->setPC(allocatePC());
    isa->decode(op);
    
    sequence.addInstruction(std::move(isa));
}

void IsaSequenceBuilder::addJump(uint64_t targetPC) {
    auto isa = std::make_unique<JumpIsa>();
    isa->setPC(allocatePC());
    isa->setJumpTarget(targetPC);
    sequence.addInstruction(std::move(isa));
}

void IsaSequenceBuilder::addConditionalJump(mlir::Value condition, 
                                             uint64_t truePC, uint64_t falsePC) {
    auto isa = std::make_unique<ConditionalJumpIsa>();
    isa->setPC(allocatePC());
    isa->setCondition(condition);
    isa->setJumpTarget(truePC);
    isa->setFallthroughPC(falsePC);
    sequence.addInstruction(std::move(isa));
}

void IsaSequenceBuilder::addReturn() {
    auto isa = std::make_unique<ReturnIsa>();
    isa->setPC(allocatePC());
    sequence.addInstruction(std::move(isa));
}

}
