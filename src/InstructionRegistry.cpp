#include "InstructionRegistry.h"

namespace ascendir_parser {

void InstructionRegistry::registerHandler(const std::string& opName, ExecuteFunc handler, uint64_t latency) {
    instructions[opName] = InstructionInfo(handler, latency);
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
