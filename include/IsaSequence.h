#pragma once

#include "Isa.h"
#include "IsaRegistry.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include <vector>
#include <map>
#include <string>

namespace ascendir_parser {

class IsaSequence {
    std::vector<IsaPtr> instructions;
    std::map<uint64_t, size_t> pcToIndex;
    std::map<std::string, uint64_t> labels;
    
public:
    IsaSequence() = default;
    IsaSequence(IsaSequence&& other) noexcept = default;
    IsaSequence& operator=(IsaSequence&& other) noexcept = default;
    
    void addInstruction(IsaPtr inst);
    Isa* getInstructionByPC(uint64_t pc);
    uint64_t getNextPC(uint64_t currentPC);
    
    uint64_t allocatePC() const;
    void setLabel(const std::string& label, uint64_t pc);
    uint64_t getLabelPC(const std::string& label);
    
    size_t size() const { return instructions.size(); }
    bool isValidPC(uint64_t pc) const;
    
    void updateConditionalJump(uint64_t pc, uint64_t truePC, uint64_t falsePC);
    void updateJumpTarget(uint64_t pc, uint64_t targetPC);
    
    std::string getInstructionDescription(uint64_t pc) const;
    const Isa* getInstructionByPCConst(uint64_t pc) const;
    
    void dump();
};

class IsaSequenceBuilder {
    IsaSequence sequence;
    uint64_t nextPC = 0;
    
public:
    void buildFromMLIR(mlir::ModuleOp module);
    IsaSequence& getSequence() { return sequence; }
    
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

}
