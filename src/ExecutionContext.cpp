#include "ExecutionContext.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SmallString.h"
#include <iostream>
#include <algorithm>

namespace ascendir_parser {

void ExecutionContext::setIntValue(mlir::Value v, llvm::APInt val) {
    intValues[v.getAsOpaquePointer()] = val;
}

void ExecutionContext::setFloatValue(mlir::Value v, llvm::APFloat val) {
    auto key = v.getAsOpaquePointer();
    auto it = floatValues.find(key);
    if (it != floatValues.end()) {
        it->second = val;
    } else {
        floatValues.emplace(key, val);
    }
}

void ExecutionContext::setMemrefData(mlir::Value v, std::vector<uint8_t> data) {
    memrefData[v.getAsOpaquePointer()] = std::move(data);
}

llvm::APInt ExecutionContext::getIntValue(mlir::Value v) {
    auto it = intValues.find(v.getAsOpaquePointer());
    if (it == intValues.end()) {
        return llvm::APInt(64, 0);
    }
    return it->second;
}

llvm::APFloat ExecutionContext::getFloatValue(mlir::Value v) {
    auto it = floatValues.find(v.getAsOpaquePointer());
    if (it == floatValues.end()) {
        return llvm::APFloat(0.0);
    }
    return it->second;
}

std::vector<uint8_t>& ExecutionContext::getMemrefData(mlir::Value v) {
    auto key = v.getAsOpaquePointer();
    if (memrefData.find(key) == memrefData.end()) {
        memrefData[key] = std::vector<uint8_t>();
    }
    return memrefData[key];
}

bool ExecutionContext::hasIntValue(mlir::Value v) {
    return intValues.find(v.getAsOpaquePointer()) != intValues.end();
}

bool ExecutionContext::hasFloatValue(mlir::Value v) {
    return floatValues.find(v.getAsOpaquePointer()) != floatValues.end();
}

bool ExecutionContext::hasMemrefData(mlir::Value v) {
    return memrefData.find(v.getAsOpaquePointer()) != memrefData.end();
}

void ExecutionContext::pushCallFrame(uint64_t returnPC) {
    CallFrame frame;
    frame.returnPC = returnPC;
    frame.savedIntValues = intValues;
    frame.savedFloatValues = floatValues;
    callStack.push_back(frame);
}

uint64_t ExecutionContext::popCallFrame() {
    if (callStack.empty()) {
        return 0;
    }
    
    CallFrame frame = callStack.back();
    callStack.pop_back();
    
    intValues = frame.savedIntValues;
    floatValues = frame.savedFloatValues;
    
    return frame.returnPC;
}

uint64_t ExecutionContext::dispatchTask(ExecutionUnitType unit, uint64_t duration, 
                                         const std::string& opName, uint64_t instPC) {
    PendingTask task;
    task.taskId = nextTaskId++;
    task.startCycle = cycle;
    task.duration = duration;
    task.unit = unit;
    task.opName = opName;
    task.pc = instPC;
    
    activeTasks.push_back(task);
    
    auto& execUnit = executionUnits.at(unit);
    execUnit.dispatchTask(task);
    
    return task.taskId;
}

void ExecutionContext::updateExecutionUnits() {
    for (auto& pair : executionUnits) {
        pair.second.update(cycle);
    }
    
    activeTasks.erase(
        std::remove_if(activeTasks.begin(), activeTasks.end(),
            [this](const PendingTask& task) {
                return task.isComplete(cycle);
            }),
        activeTasks.end());
}

bool ExecutionContext::isUnitBusy(ExecutionUnitType unit) const {
    auto it = executionUnits.find(unit);
    if (it == executionUnits.end()) {
        return false;
    }
    return it->second.isBusy(cycle);
}

uint64_t ExecutionContext::getUnitBusyUntil(ExecutionUnitType unit) const {
    auto it = executionUnits.find(unit);
    if (it == executionUnits.end()) {
        return cycle;
    }
    return it->second.getBusyUntilCycle();
}

bool ExecutionContext::allUnitsIdle() const {
    for (const auto& pair : executionUnits) {
        if (pair.second.isBusy(cycle)) {
            return false;
        }
    }
    return true;
}

uint64_t ExecutionContext::getEarliestCompletionCycle() const {
    uint64_t earliest = UINT64_MAX;
    for (const auto& pair : executionUnits) {
        if (pair.second.isBusy(cycle)) {
            uint64_t busyUntil = pair.second.getBusyUntilCycle();
            if (busyUntil < earliest) {
                earliest = busyUntil;
            }
        }
    }
    return earliest == UINT64_MAX ? cycle : earliest;
}

void ExecutionContext::advanceToCompletion(ExecutionUnitType unit) {
    uint64_t busyUntil = getUnitBusyUntil(unit);
    if (busyUntil > cycle) {
        cycle = busyUntil;
        updateExecutionUnits();
    }
}

void ExecutionContext::dump() {
    llvm::outs() << "=== Execution Context ===\n";
    llvm::outs() << "PC: " << pc << "\n";
    llvm::outs() << "Cycle: " << cycle << "\n";
    llvm::outs() << "Halted: " << (halted ? "true" : "false") << "\n";
    llvm::outs() << "Call Stack Depth: " << callStack.size() << "\n";
    
    llvm::outs() << "\nExecution Units:\n";
    for (const auto& pair : executionUnits) {
        llvm::outs() << "  " << executionUnitToString(pair.first) 
                     << ": busy=" << (pair.second.isBusy(cycle) ? "yes" : "no")
                     << ", busyUntil=" << pair.second.getBusyUntilCycle() << "\n";
    }
    
    llvm::outs() << "\nActive Tasks (" << activeTasks.size() << "):\n";
    for (const auto& task : activeTasks) {
        llvm::outs() << "  Task " << task.taskId << ": " << task.opName
                     << " on " << executionUnitToString(task.unit)
                     << ", complete at cycle " << task.completeCycle() << "\n";
    }
    
    llvm::outs() << "\nInteger Values:\n";
    for (const auto& kv : intValues) {
        llvm::outs() << "  " << kv.second << "\n";
    }
    
    llvm::outs() << "\nFloat Values:\n";
    for (const auto& kv : floatValues) {
        llvm::SmallString<16> str;
        kv.second.toString(str);
        llvm::outs() << "  " << str << "\n";
    }
}

}
