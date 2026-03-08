#pragma once

#include "Instruction.h"
#include "mlir/IR/BuiltinOps.h"
#include <vector>
#include <map>
#include <string>

namespace ascendir_parser {

class InstructionSequence {
    std::vector<Instruction> instructions;
    std::map<uint64_t, size_t> pcToIndex;
    std::map<std::string, uint64_t> labels;
    
public:
    void addInstruction(const Instruction& inst);
    Instruction* getInstructionByPC(uint64_t pc);
    uint64_t getNextPC(uint64_t currentPC);
    
    uint64_t allocatePC();
    void setLabel(const std::string& label, uint64_t pc);
    uint64_t getLabelPC(const std::string& label);
    
    size_t size() const { return instructions.size(); }
    bool isValidPC(uint64_t pc) const;
    
    void updateConditionalJump(uint64_t pc, uint64_t truePC, uint64_t falsePC);
    void updateJumpTarget(uint64_t pc, uint64_t targetPC);
    
    std::string getInstructionDescription(uint64_t pc) const;
    const Instruction* getInstructionByPCConst(uint64_t pc) const;
    
    void dump();
};

}
