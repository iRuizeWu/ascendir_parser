#pragma once

#include "ExecutionUnit.h"
#include <string>
#include <map>
#include <cstdint>

namespace ascendir_parser {

struct ExternalFunctionInfo {
    uint64_t latency;
    ExecutionUnitType executionUnit;
    std::string description;
    
    ExternalFunctionInfo() 
        : latency(10), executionUnit(ExecutionUnitType::Scalar) {}
    
    ExternalFunctionInfo(uint64_t lat, ExecutionUnitType unit, 
                         const std::string& desc = "")
        : latency(lat), executionUnit(unit), description(desc) {}
};

class ExternalFunctionConfig {
    std::map<std::string, ExternalFunctionInfo> functions;
    ExternalFunctionInfo defaultInfo;
    
public:
    ExternalFunctionConfig() {
        defaultInfo.latency = 10;
        defaultInfo.executionUnit = ExecutionUnitType::Scalar;
    }
    
    bool loadFromFile(const std::string& filepath);
    bool loadFromYaml(const std::string& yamlContent);
    
    void setFunctionCycle(const std::string& funcName, uint64_t cycle);
    void setFunctionInfo(const std::string& funcName, const ExternalFunctionInfo& info);
    void setDefaultLatency(uint64_t latency);
    void setDefaultUnit(ExecutionUnitType unit);
    
    bool hasFunction(const std::string& funcName) const;
    ExternalFunctionInfo getFunctionInfo(const std::string& funcName) const;
    
    void clear();
    void dump() const;
    
    static ExternalFunctionConfig& getGlobalConfig();
    
private:
    ExecutionUnitType parseExecutionUnit(const std::string& unitStr) const;
};

}
