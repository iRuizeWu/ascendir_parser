#pragma once

#include "ExecutionUnit.h"
#include "mlir/IR/Value.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APFloat.h"
#include <map>
#include <vector>
#include <cstdint>
#include <queue>

namespace ascendir_parser {

class ExecutionContext {
    std::map<void*, llvm::APInt> intValues;
    std::map<void*, llvm::APFloat> floatValues;
    std::map<void*, std::vector<uint8_t>> memrefData;
    
    uint64_t pc;
    bool halted;
    uint64_t cycle;
    
    struct CallFrame {
        uint64_t returnPC;
        std::map<void*, llvm::APInt> savedIntValues;
        std::map<void*, llvm::APFloat> savedFloatValues;
    };
    std::vector<CallFrame> callStack;
    
    std::map<ExecutionUnitType, ExecutionUnit> executionUnits;
    std::vector<PendingTask> activeTasks;
    uint64_t nextTaskId;
    
    bool waitForUnits;
    ExecutionUnitType waitingForUnit;
    
public:
    ExecutionContext() : pc(0), halted(false), cycle(0), nextTaskId(0), 
                         waitForUnits(false), waitingForUnit(ExecutionUnitType::Scalar) {
        executionUnits.emplace(ExecutionUnitType::Scalar, ExecutionUnit(ExecutionUnitType::Scalar));
        executionUnits.emplace(ExecutionUnitType::MTE, ExecutionUnit(ExecutionUnitType::MTE));
        executionUnits.emplace(ExecutionUnitType::Cube, ExecutionUnit(ExecutionUnitType::Cube));
        executionUnits.emplace(ExecutionUnitType::Vec, ExecutionUnit(ExecutionUnitType::Vec));
    }
    
    void setIntValue(mlir::Value v, llvm::APInt val);
    void setFloatValue(mlir::Value v, llvm::APFloat val);
    void setMemrefData(mlir::Value v, std::vector<uint8_t> data);
    
    llvm::APInt getIntValue(mlir::Value v);
    llvm::APFloat getFloatValue(mlir::Value v);
    std::vector<uint8_t>& getMemrefData(mlir::Value v);
    
    bool hasIntValue(mlir::Value v);
    bool hasFloatValue(mlir::Value v);
    bool hasMemrefData(mlir::Value v);
    
    void setPC(uint64_t newPC) { pc = newPC; }
    uint64_t getPC() const { return pc; }
    void nextPC() { pc++; }
    
    void halt() { halted = true; }
    bool isHalted() const { return halted; }
    
    uint64_t getCycle() const { return cycle; }
    void advanceCycle(uint64_t cycles) { cycle += cycles; }
    void setCycle(uint64_t c) { cycle = c; }
    
    void pushCallFrame(uint64_t returnPC);
    uint64_t popCallFrame();
    bool hasCallFrames() const { return !callStack.empty(); }
    
    ExecutionUnit& getExecutionUnit(ExecutionUnitType type) {
        return executionUnits.at(type);
    }
    
    uint64_t dispatchTask(ExecutionUnitType unit, uint64_t duration, 
                          const std::string& opName, uint64_t pc);
    
    void updateExecutionUnits();
    
    bool isUnitBusy(ExecutionUnitType unit) const;
    
    uint64_t getUnitBusyUntil(ExecutionUnitType unit) const;
    
    void waitForUnit(ExecutionUnitType unit) {
        waitForUnits = true;
        waitingForUnit = unit;
    }
    
    void clearWait() {
        waitForUnits = false;
    }
    
    bool shouldWait() const { return waitForUnits; }
    
    ExecutionUnitType getWaitingUnit() const { return waitingForUnit; }
    
    bool allUnitsIdle() const;
    
    uint64_t getEarliestCompletionCycle() const;
    
    void advanceToCompletion(ExecutionUnitType unit);
    
    const std::vector<PendingTask>& getActiveTasks() const { return activeTasks; }
    
    void dump();
};

}
