#include "ExternalFunctionConfig.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sstream>
#include <iostream>

namespace ascendir_parser {

ExternalFunctionConfig& ExternalFunctionConfig::getGlobalConfig() {
    static ExternalFunctionConfig instance;
    return instance;
}

ExecutionUnitType ExternalFunctionConfig::parseExecutionUnit(const std::string& unitStr) const {
    if (unitStr == "Scalar") return ExecutionUnitType::Scalar;
    if (unitStr == "MTE") return ExecutionUnitType::MTE;
    if (unitStr == "Cube") return ExecutionUnitType::Cube;
    if (unitStr == "Vec") return ExecutionUnitType::Vec;
    return ExecutionUnitType::Scalar;
}

bool ExternalFunctionConfig::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open config file: " << filepath << "\n";
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return loadFromYaml(buffer.str());
}

bool ExternalFunctionConfig::loadFromYaml(const std::string& yamlContent) {
    try {
        YAML::Node root = YAML::Load(yamlContent);
        
        if (root["external_functions"]) {
            const YAML::Node& funcs = root["external_functions"];
            for (const auto& it : funcs) {
                std::string funcName = it.first.as<std::string>();
                const YAML::Node& info = it.second;
                
                ExternalFunctionInfo funcInfo;
                
                if (info["latency"]) {
                    funcInfo.latency = info["latency"].as<uint64_t>();
                }
                
                if (info["execution_unit"]) {
                    funcInfo.executionUnit = parseExecutionUnit(info["execution_unit"].as<std::string>());
                }
                
                if (info["description"]) {
                    funcInfo.description = info["description"].as<std::string>();
                }
                
                functions[funcName] = funcInfo;
            }
        }
        
        if (root["default_latency"]) {
            defaultInfo.latency = root["default_latency"].as<uint64_t>();
        }
        
        if (root["default_unit"]) {
            defaultInfo.executionUnit = parseExecutionUnit(root["default_unit"].as<std::string>());
        }
        
        return true;
    } catch (const YAML::Exception& e) {
        std::cerr << "YAML parsing error: " << e.what() << "\n";
        return false;
    }
}

void ExternalFunctionConfig::setFunctionCycle(const std::string& funcName, uint64_t cycle) {
    functions[funcName].latency = cycle;
}

void ExternalFunctionConfig::setFunctionInfo(const std::string& funcName, const ExternalFunctionInfo& info) {
    functions[funcName] = info;
}

void ExternalFunctionConfig::setDefaultLatency(uint64_t latency) {
    defaultInfo.latency = latency;
}

void ExternalFunctionConfig::setDefaultUnit(ExecutionUnitType unit) {
    defaultInfo.executionUnit = unit;
}

bool ExternalFunctionConfig::hasFunction(const std::string& funcName) const {
    return functions.find(funcName) != functions.end();
}

ExternalFunctionInfo ExternalFunctionConfig::getFunctionInfo(const std::string& funcName) const {
    auto it = functions.find(funcName);
    if (it != functions.end()) {
        return it->second;
    }
    return defaultInfo;
}

void ExternalFunctionConfig::clear() {
    functions.clear();
    defaultInfo = ExternalFunctionInfo();
}

void ExternalFunctionConfig::dump() const {
    std::cout << "=== External Function Configuration ===\n";
    std::cout << "Default latency: " << defaultInfo.latency << "\n";
    std::cout << "Default unit: " << executionUnitToString(defaultInfo.executionUnit) << "\n";
    
    std::cout << "\nConfigured functions:\n";
    for (const auto& pair : functions) {
        std::cout << "  " << pair.first << ": "
                  << "latency=" << pair.second.latency << ", "
                  << "unit=" << executionUnitToString(pair.second.executionUnit);
        if (!pair.second.description.empty()) {
            std::cout << ", desc=\"" << pair.second.description << "\"";
        }
        std::cout << "\n";
    }
}

}
