#pragma once

#include "mlir/IR/Value.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APFloat.h"
#include <map>
#include <vector>
#include <cstdint>

namespace ascendir_parser {

class ExecutionContext {
    std::map<void*, llvm::APInt> intValues;
    std::map<void*, llvm::APFloat> floatValues;
    std::map<void*, std::vector<uint8_t>> memrefData;
    
    struct CallFrame {
        uint64_t returnPC;
        std::map<void*, llvm::APInt> savedIntValues;
        std::map<void*, llvm::APFloat> savedFloatValues;
    };
    std::vector<CallFrame> callStack;
    
    bool halted;
    
public:
    ExecutionContext() : halted(false) {}
    
    void setIntValue(mlir::Value v, llvm::APInt val);
    void setFloatValue(mlir::Value v, llvm::APFloat val);
    void setMemrefData(mlir::Value v, std::vector<uint8_t> data);
    
    llvm::APInt getIntValue(mlir::Value v);
    llvm::APFloat getFloatValue(mlir::Value v);
    std::vector<uint8_t>& getMemrefData(mlir::Value v);
    
    bool hasIntValue(mlir::Value v);
    bool hasFloatValue(mlir::Value v);
    bool hasMemrefData(mlir::Value v);
    
    void halt() { halted = true; }
    bool isHalted() const { return halted; }
    void clearHalt() { halted = false; }
    
    void pushCallFrame(uint64_t returnPC);
    uint64_t popCallFrame();
    bool hasCallFrames() const { return !callStack.empty(); }
    
    void dump();
};

}
