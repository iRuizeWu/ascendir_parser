#include "ExecutionContext.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SmallString.h"
#include <iostream>

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

void ExecutionContext::dump() {
    llvm::outs() << "=== Execution Context ===\n";
    llvm::outs() << "Halted: " << (halted ? "true" : "false") << "\n";
    llvm::outs() << "Call Stack Depth: " << callStack.size() << "\n";
    
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
