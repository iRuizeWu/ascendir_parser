#pragma once

#include "ExecutionContext.h"
#include <functional>
#include <string>
#include <map>

namespace ascendir_parser {

struct InstructionInfo {
    using ExecuteFunc = std::function<void(mlir::Operation*, ExecutionContext&)>;
    ExecuteFunc handler;
    uint64_t latency;
    
    InstructionInfo() : latency(1) {}
    InstructionInfo(ExecuteFunc h, uint64_t lat = 1) : handler(h), latency(lat) {}
};

class InstructionRegistry {
public:
    using ExecuteFunc = std::function<void(mlir::Operation*, ExecutionContext&)>;
    
    void registerHandler(const std::string& opName, ExecuteFunc handler, uint64_t latency = 1);
    InstructionInfo* getInstructionInfo(const std::string& opName);
    bool hasHandler(const std::string& opName);
    
private:
    std::map<std::string, InstructionInfo> instructions;
};

InstructionRegistry& getGlobalRegistry();

#define REGISTER_INSTRUCTION(opName, handler) \
    getGlobalRegistry().registerHandler(opName, handler)

#define REGISTER_INSTRUCTION_WITH_LATENCY(opName, handler, latency) \
    getGlobalRegistry().registerHandler(opName, handler, latency)

void registerAllInstructions();

}
