#include "InstructionRegistry.h"

namespace ascendir_parser {

void InstructionRegistry::registerHandler(const std::string& opName, ExecuteFunc handler, 
                                          uint64_t latency, ExecutionUnitType unit) {
    InstructionInfo info(handler, latency, unit);
    instructions[opName] = info;
}

void InstructionRegistry::registerHandlerWithDataSize(const std::string& opName, ExecuteFunc handler,
                                                      DataSizeFunc dataSizeFunc,
                                                      const ComponentLatencyModel& model,
                                                      ExecutionUnitType unit) {
    InstructionInfo info;
    info.handler = handler;
    info.targetUnit = unit;
    info.dataSizeDependent = true;
    info.dataSizeCalculator = dataSizeFunc;
    info.latencyModel = model;
    info.latency = model.baseLatency;
    instructions[opName] = info;
}

InstructionInfo* InstructionRegistry::getInstructionInfo(const std::string& opName) {
    auto it = instructions.find(opName);
    if (it == instructions.end()) {
        return nullptr;
    }
    return &it->second;
}

bool InstructionRegistry::hasHandler(const std::string& opName) {
    return instructions.find(opName) != instructions.end();
}

InstructionRegistry& getGlobalRegistry() {
    static InstructionRegistry registry;
    return registry;
}

void registerArithInstructions();
void registerFuncInstructions();
void registerHivmInstructions();

void registerAllInstructions() {
    registerArithInstructions();
    registerFuncInstructions();
    registerHivmInstructions();
}

}
