#pragma once

#include "Isa.h"
#include <map>
#include <string>
#include <memory>

namespace ascendir_parser {

class IsaRegistry {
public:
    using IsaFactory = std::function<IsaPtr()>;
    
    void registerIsa(const std::string& opName, IsaFactory factory, IsaName isaName);
    
    IsaPtr createIsa(const std::string& opName);
    bool hasIsa(const std::string& opName);
    
    static IsaRegistry& getGlobalRegistry();
    
private:
    std::map<std::string, IsaFactory> isaRegistry;
    std::map<std::string, IsaName> isaNameMap;  // 新增：opName到IsaName的映射
};

// 修改宏：同时注册IsaName
#define REGISTER_ISA(opName, IsaClass, isaNameEnum) \
    IsaRegistry::getGlobalRegistry().registerIsa(opName, \
        []() -> IsaPtr { \
            auto isa = std::make_unique<IsaClass>(); \
            isa->setIsaName(isaNameEnum); \
            return isa; \
        }, isaNameEnum)

void registerAllIsas();

}