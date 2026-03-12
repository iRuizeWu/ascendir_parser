#include "IsaRegistry.h"
#include "Instructions/ArithIsas.h"
#include "Instructions/FuncIsas.h"
#include "Instructions/HivmIsas.h"
#include "Instructions/ControlFlowIsas.h"

namespace ascendir_parser {

void IsaRegistry::registerIsa(const std::string& opName, IsaFactory factory, IsaName isaName) {
    isaRegistry[opName] = factory;
    isaNameMap[opName] = isaName;  // 同时记录IsaName映射
}

IsaPtr IsaRegistry::createIsa(const std::string& opName) {
    auto it = isaRegistry.find(opName);
    if (it == isaRegistry.end()) {
        return nullptr;
    }
    return it->second();
}

bool IsaRegistry::hasIsa(const std::string& opName) {
    return isaRegistry.find(opName) != isaRegistry.end();
}

IsaRegistry& IsaRegistry::getGlobalRegistry() {
    static IsaRegistry registry;
    return registry;
}

void registerAllIsas() {
    registerArithIsas();
    registerFuncIsas();
    registerHivmIsas();
}

}