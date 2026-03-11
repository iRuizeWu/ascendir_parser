#pragma once

#include "ExecutionContext.h"
#include "ExecutionUnit.h"
#include <functional>
#include <string>
#include <map>

namespace ascendir_parser {

struct InstructionInfo {
    using ExecuteFunc = std::function<void(mlir::Operation*, ExecutionContext&)>;
    using DataSizeFunc = std::function<size_t(mlir::Operation*, ExecutionContext&)>;
    
    ExecuteFunc handler;
    uint64_t latency;
    ExecutionUnitType targetUnit;
    bool dataSizeDependent;
    DataSizeFunc dataSizeCalculator;
    ComponentLatencyModel latencyModel;
    
    InstructionInfo() 
        : latency(1), targetUnit(ExecutionUnitType::Scalar), 
          dataSizeDependent(false) {}
    
    InstructionInfo(ExecuteFunc h, uint64_t lat = 1, 
                    ExecutionUnitType unit = ExecutionUnitType::Scalar)
        : handler(h), latency(lat), targetUnit(unit), 
          dataSizeDependent(false) {}
    
    uint64_t calculateLatency(size_t dataSize) const {
        if (!dataSizeDependent) {
            return latency;
        }
        return latencyModel.calculateLatency(dataSize);
    }
};

class InstructionRegistry {
public:
    using ExecuteFunc = std::function<void(mlir::Operation*, ExecutionContext&)>;
    using DataSizeFunc = std::function<size_t(mlir::Operation*, ExecutionContext&)>;
    
    void registerHandler(const std::string& opName, ExecuteFunc handler, 
                        uint64_t latency = 1,
                        ExecutionUnitType unit = ExecutionUnitType::Scalar);
    
    void registerHandlerWithDataSize(const std::string& opName, ExecuteFunc handler,
                                     DataSizeFunc dataSizeFunc,
                                     const ComponentLatencyModel& model,
                                     ExecutionUnitType unit);
    
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

#define REGISTER_INSTRUCTION_ON_UNIT(opName, handler, latency, unit) \
    getGlobalRegistry().registerHandler(opName, handler, latency, unit)

#define REGISTER_INSTRUCTION_WITH_DATA_SIZE(opName, handler, dataSizeFunc, model, unit) \
    getGlobalRegistry().registerHandlerWithDataSize(opName, handler, dataSizeFunc, model, unit)

void registerAllInstructions();

}
