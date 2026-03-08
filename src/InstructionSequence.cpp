#include "InstructionSequence.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>

namespace ascendir_parser {

void InstructionSequence::addInstruction(const Instruction& inst) {
    size_t index = instructions.size();
    instructions.push_back(inst);
    pcToIndex[inst.pc] = index;
}

Instruction* InstructionSequence::getInstructionByPC(uint64_t pc) {
    auto it = pcToIndex.find(pc);
    if (it == pcToIndex.end()) {
        return nullptr;
    }
    return &instructions[it->second];
}

uint64_t InstructionSequence::getNextPC(uint64_t currentPC) {
    auto it = pcToIndex.find(currentPC);
    if (it == pcToIndex.end()) {
        return currentPC + 1;
    }
    size_t index = it->second;
    if (index + 1 < instructions.size()) {
        return instructions[index + 1].pc;
    }
    return currentPC + 1;
}

uint64_t InstructionSequence::allocatePC() {
    uint64_t pc = instructions.empty() ? 0 : instructions.back().pc + 1;
    return pc;
}

void InstructionSequence::setLabel(const std::string& label, uint64_t pc) {
    labels[label] = pc;
}

uint64_t InstructionSequence::getLabelPC(const std::string& label) {
    auto it = labels.find(label);
    if (it == labels.end()) {
        return 0;
    }
    return it->second;
}

bool InstructionSequence::isValidPC(uint64_t pc) const {
    return pcToIndex.find(pc) != pcToIndex.end();
}

void InstructionSequence::updateConditionalJump(uint64_t pc, uint64_t truePC, uint64_t falsePC) {
    auto it = pcToIndex.find(pc);
    if (it != pcToIndex.end()) {
        instructions[it->second].jumpTarget = truePC;
        instructions[it->second].fallthroughPC = falsePC;
    }
}

void InstructionSequence::updateJumpTarget(uint64_t pc, uint64_t targetPC) {
    auto it = pcToIndex.find(pc);
    if (it != pcToIndex.end()) {
        instructions[it->second].jumpTarget = targetPC;
    }
}

const Instruction* InstructionSequence::getInstructionByPCConst(uint64_t pc) const {
    auto it = pcToIndex.find(pc);
    if (it == pcToIndex.end()) {
        return nullptr;
    }
    return &instructions[it->second];
}

std::string InstructionSequence::getInstructionDescription(uint64_t pc) const {
    const Instruction* inst = getInstructionByPCConst(pc);
    if (!inst) {
        return "Unknown instruction";
    }
    
    std::string result;
    llvm::raw_string_ostream os(result);
    
    switch (inst->kind) {
        case Instruction::Kind::Normal:
            os << "Normal: ";
            if (inst->mlirOp) {
                inst->mlirOp->print(os);
            } else if (!inst->sourceLocation.empty()) {
                os << inst->sourceLocation;
            }
            break;
        case Instruction::Kind::Jump:
            os << "Jump -> PC " << inst->jumpTarget;
            break;
        case Instruction::Kind::ConditionalJump:
            os << "ConditionalJump: true=" << inst->jumpTarget 
               << ", false=" << inst->fallthroughPC;
            break;
        case Instruction::Kind::Return:
            os << "Return";
            break;
        case Instruction::Kind::ForInit:
            os << "ForInit: iv=" << inst->forIV;
            break;
        case Instruction::Kind::ForCondition:
            os << "ForCondition: iv < ub? true->" << inst->jumpTarget 
               << ", false->" << inst->fallthroughPC;
            break;
        case Instruction::Kind::ForIncrement:
            os << "ForIncrement: iv += step";
            break;
        case Instruction::Kind::IfCondition:
            os << "IfCondition: cond? true->" << inst->jumpTarget 
               << ", false->" << inst->fallthroughPC;
            break;
    }
    
    return result;
}

void InstructionSequence::dump() {
    llvm::outs() << "=== Instruction Sequence ===\n";
    llvm::outs() << "PC  | Kind              | Details\n";
    llvm::outs() << "----|-------------------|--------\n";
    
    for (const auto& inst : instructions) {
        llvm::outs() << inst.pc << " | ";
        
        switch (inst.kind) {
            case Instruction::Kind::Normal:
                llvm::outs() << "Normal            | ";
                if (inst.mlirOp) {
                    inst.mlirOp->print(llvm::outs());
                }
                break;
            case Instruction::Kind::Jump:
                llvm::outs() << "Jump              | target=" << inst.jumpTarget;
                break;
            case Instruction::Kind::ConditionalJump:
                llvm::outs() << "ConditionalJump   | true=" << inst.jumpTarget 
                            << ", false=" << inst.fallthroughPC;
                break;
            case Instruction::Kind::Return:
                llvm::outs() << "Return            |";
                break;
            case Instruction::Kind::ForInit:
                llvm::outs() << "ForInit           | iv, lb, ub, step";
                break;
            case Instruction::Kind::ForCondition:
                llvm::outs() << "ForCondition      | true=" << inst.jumpTarget 
                            << ", false=" << inst.fallthroughPC;
                break;
            case Instruction::Kind::ForIncrement:
                llvm::outs() << "ForIncrement      | iv += step";
                break;
            case Instruction::Kind::IfCondition:
                llvm::outs() << "IfCondition       | true=" << inst.jumpTarget 
                            << ", false=" << inst.fallthroughPC;
                break;
        }
        llvm::outs() << "\n";
    }
}

}
