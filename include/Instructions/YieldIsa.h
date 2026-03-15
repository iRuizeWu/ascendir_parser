#pragma once

#include "Isa.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "llvm/Support/raw_ostream.h"
#include <optional>

namespace ascendir_parser {

class YieldAssignIsa : public Isa {
    std::string locationInfo;
    mutable std::optional<llvm::APInt> cachedIntValue;
    mutable std::optional<llvm::APFloat> cachedFloatValue;
    
public:
    YieldAssignIsa() {
        kind = Kind::Normal;
        isaName = IsaName::YieldAssign;
    }
    
    void setLocationInfo(const std::string& loc) {
        locationInfo = loc;
    }
    
    void execute(ExecutionContext& ctx) override {
        if (condition && forIV) {
            if (ctx.hasIntValue(condition)) {
                llvm::APInt val = ctx.getIntValue(condition);
                cachedIntValue = val;
                ctx.setIntValue(forIV, val);
            } else if (ctx.hasFloatValue(condition)) {
                llvm::APFloat val = ctx.getFloatValue(condition);
                cachedFloatValue = val;
                ctx.setFloatValue(forIV, val);
            }
        }
    }
    
    std::string getDescription() const override {
        std::string result = "yield.assign";
        if (!locationInfo.empty()) {
            result += " [" + locationInfo + "]";
        }
        if (condition && forIV) {
            result += " (";
            if (auto blockArg = llvm::dyn_cast<mlir::BlockArgument>(condition)) {
                result += "arg#" + std::to_string(blockArg.getArgNumber());
            } else if (auto definingOp = condition.getDefiningOp()) {
                result += definingOp->getName().getStringRef().str();
            } else {
                result += "value";
            }
            
            if (cachedIntValue.has_value()) {
                result += "=" + std::to_string(cachedIntValue->getSExtValue());
            } else if (cachedFloatValue.has_value()) {
                llvm::SmallVector<char, 16> buffer;
                cachedFloatValue->toString(buffer);
                result += "=" + std::string(buffer.data(), buffer.size());
            }
            
            result += " -> ";
            if (auto blockArg = llvm::dyn_cast<mlir::BlockArgument>(forIV)) {
                result += "iterArg#" + std::to_string(blockArg.getArgNumber());
            } else {
                result += "target";
            }
            result += ")";
        }
        return result;
    }
};

class IfYieldIsa : public Isa {
    std::string locationInfo;
    std::vector<mlir::Value> ifResults;
    mutable std::vector<std::string> cachedValues;
    
public:
    IfYieldIsa() {
        kind = Kind::Normal;
        isaName = IsaName::IfYield;
    }
    
    void setLocationInfo(const std::string& loc) {
        locationInfo = loc;
    }
    
    void setIfResults(mlir::ValueRange results) {
        ifResults.clear();
        for (auto val : results) {
            ifResults.push_back(val);
        }
    }
    
    void execute(ExecutionContext& ctx) override {
        cachedValues.clear();
        for (auto val : ifResults) {
            if (ctx.hasIntValue(val)) {
                auto intVal = ctx.getIntValue(val);
                cachedValues.push_back(std::to_string(intVal.getSExtValue()));
            } else if (ctx.hasFloatValue(val)) {
                auto floatVal = ctx.getFloatValue(val);
                llvm::SmallVector<char, 16> buffer;
                floatVal.toString(buffer);
                cachedValues.push_back(std::string(buffer.data(), buffer.size()));
            } else {
                cachedValues.push_back("?");
            }
        }
    }
    
    std::string getDescription() const override {
        std::string result = "if.yield";
        if (!locationInfo.empty()) {
            result += " [" + locationInfo + "]";
        }
        if (!ifResults.empty()) {
            result += " (";
            for (size_t i = 0; i < ifResults.size(); ++i) {
                if (i > 0) result += ", ";
                auto val = ifResults[i];
                if (auto blockArg = llvm::dyn_cast<mlir::BlockArgument>(val)) {
                    result += "arg#" + std::to_string(blockArg.getArgNumber());
                } else if (auto definingOp = val.getDefiningOp()) {
                    result += definingOp->getName().getStringRef().str();
                } else {
                    result += "value";
                }
                
                if (i < cachedValues.size() && cachedValues[i] != "?") {
                    result += "=" + cachedValues[i];
                }
            }
            result += ")";
        }
        return result;
    }
};

}
