#pragma once

#include "Isa.h"
#include "ExternalFunctionConfig.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/Support/raw_ostream.h"

namespace ascendir_parser {

class FuncCallIsa : public Isa {
    std::string calleeName;
    bool isExternal;
    
public:
    FuncCallIsa() : isExternal(false) {
        isaName = IsaName::FuncCall;
    }
    
    bool decode(mlir::Operation* op) override {
        mlirOp = op;
        if (op) {
            opName = op->getName().getStringRef().str();
        }
        
        if (auto callOp = llvm::dyn_cast<mlir::func::CallOp>(op)) {
            calleeName = callOp.getCallee().str();
            isExternal = checkIfExternal(op);
        }
        return true;
    }
    
    bool checkIfExternal(mlir::Operation* op) {
        auto module = op->getParentOfType<mlir::ModuleOp>();
        if (!module) return true;
        
        for (auto& funcOp : module.getBody()->getOperations()) {
            if (auto func = llvm::dyn_cast<mlir::func::FuncOp>(funcOp)) {
                if (func.getSymName().str() == calleeName) {
                    if (func.getBody().empty()) {
                        return true;
                    }
                    return false;
                }
            }
        }
        return true;
    }
    
    bool isExternalFunction() const { return isExternal; }
    const std::string& getCalleeName() const { return calleeName; }
    
    uint64_t getLatency() const override {
        if (isExternal) {
            auto& config = ExternalFunctionConfig::getGlobalConfig();
            return config.getFunctionInfo(calleeName).latency;
        }
        return 5;
    }
    
    ExecutionUnitType getExecutionUnit() const override {
        if (isExternal) {
            auto& config = ExternalFunctionConfig::getGlobalConfig();
            return config.getFunctionInfo(calleeName).executionUnit;
        }
        return ExecutionUnitType::Scalar;
    }
    
    HardwareCharacteristics getHardwareCharacteristics(ExecutionContext& ctx) override {
        HardwareCharacteristics hw;
        hw.latency = getLatency();
        hw.targetUnit = getExecutionUnit();
        return hw;
    }
    
    void execute(ExecutionContext& ctx) override {
        if (isExternal) {
            return;
        }
    }
    
    std::string getDescription() const override {
        if (isExternal) {
            return "func.call @" + calleeName + " [external, latency=" + 
                   std::to_string(getLatency()) + "]";
        }
        return "func.call @" + calleeName;
    }
};

class FuncReturnIsa : public Isa {
public:
    FuncReturnIsa() {
        isaName = IsaName::FuncReturn;
    }
    
    uint64_t getLatency() const override { return 1; }
    
    void execute(ExecutionContext& ctx) override {
    }
    
    std::string getDescription() const override {
        return "func.return";
    }
};

inline void registerFuncIsas() {
    REGISTER_ISA("func.call", FuncCallIsa, IsaName::FuncCall);
    REGISTER_ISA("func.return", FuncReturnIsa, IsaName::FuncReturn);
}

}
