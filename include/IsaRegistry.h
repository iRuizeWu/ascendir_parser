#pragma once

#include "Isa.h"
#include <map>
#include <string>
#include <memory>

namespace ascendir_parser {

class IsaRegistry {
public:
    using IsaFactory = std::function<IsaPtr()>;
    
    void registerIsa(const std::string& opName, IsaFactory factory);
    
    IsaPtr createIsa(const std::string& opName);
    bool hasIsa(const std::string& opName);
    
    static IsaRegistry& getGlobalRegistry();
    
private:
    std::map<std::string, IsaFactory> isaRegistry;
};

#define REGISTER_ISA(opName, IsaClass) \
    IsaRegistry::getGlobalRegistry().registerIsa(opName, \
        []() -> IsaPtr { return std::make_unique<IsaClass>(); })

void registerAllIsas();

}
