#pragma once

#include "IsaSequence.h"
#include "ExecutionContext.h"
#include "ExecutionUnit.h"
#include "mlir/IR/BuiltinOps.h"
#include <map>

namespace ascendir_parser {

class IsaExecutor {
    IsaSequence sequence;
    ExecutionContext ctx;
    
    uint64_t pc;
    uint64_t cycle;
    
    std::map<ExecutionUnitType, ExecutionUnit> executionUnits;
    std::vector<PendingTask> activeTasks;
    uint64_t nextTaskId;
    
    bool verboseMode;
    bool asyncMode;
    
    bool blocked;
    ExecutionUnitType blockingUnit;
    
public:
    IsaExecutor() : pc(0), cycle(0), nextTaskId(0), verboseMode(false), 
                    asyncMode(false), blocked(false), blockingUnit(ExecutionUnitType::Scalar) {
        executionUnits.emplace(ExecutionUnitType::Scalar, ExecutionUnit(ExecutionUnitType::Scalar));
        executionUnits.emplace(ExecutionUnitType::MTE, ExecutionUnit(ExecutionUnitType::MTE));
        executionUnits.emplace(ExecutionUnitType::Cube, ExecutionUnit(ExecutionUnitType::Cube));
        executionUnits.emplace(ExecutionUnitType::Vec, ExecutionUnit(ExecutionUnitType::Vec));
    }
    
    void load(mlir::ModuleOp module);
    bool tick();
    void run();
    
    uint64_t getCurrentPC() const { return pc; }
    uint64_t getCurrentCycle() const { return cycle; }
    bool isHalted() const { return ctx.isHalted(); }
    bool isBlocked() const { return blocked; }
    
    ExecutionContext& getContext() { return ctx; }
    IsaSequence& getSequence() { return sequence; }
    
    void setVerbose(bool verbose) { verboseMode = verbose; }
    void setAsyncMode(bool async) { asyncMode = async; }
    
    bool isUnitBusy(ExecutionUnitType unit) const;
    
private:
    void executeIsa(Isa* isa);
    
    uint64_t dispatchTask(ExecutionUnitType unit, uint64_t duration, 
                          const std::string& opName, uint64_t instPC);
    void updateExecutionUnits();
};

}
