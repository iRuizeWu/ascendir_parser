#include "IsaExecutor.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>

namespace ascendir_parser {

void IsaExecutor::load(mlir::ModuleOp module) {
    IsaSequenceBuilder builder;
    builder.buildFromMLIR(module);
    sequence = std::move(builder.getSequence());
    
    pc = 0;
    cycle = 0;
    blocked = false;
    nextTaskId = 0;
    activeTasks.clear();
    
    ctx.clearHalt();
    
    for (auto& pair : executionUnits) {
        pair.second = ExecutionUnit(pair.first);
    }
    
    if (sequence.size() == 0) {
        ctx.halt();
    }
}

bool IsaExecutor::tick() {
    if (ctx.isHalted()) {
        return false;
    }
    
    cycle++;
    
    updateExecutionUnits();
    
    if (blocked) {
        if (!isUnitBusy(blockingUnit)) {
            blocked = false;
            if (verboseMode) {
                llvm::outs() << "Cycle[" << cycle << "] Unblocked, unit " 
                             << executionUnitToString(blockingUnit) << " is now free\n";
            }
        } else {
            if (verboseMode) {
                llvm::outs() << "Cycle[" << cycle << "] Blocked waiting for " 
                             << executionUnitToString(blockingUnit) << "\n";
            }
            return true;
        }
    }
    
    Isa* isa = sequence.getInstructionByPC(pc);
    if (!isa) {
        ctx.halt();
        return false;
    }
    
    if (verboseMode) {
        llvm::outs() << "Cycle[" << cycle << "] PC[" << pc << "] "
                     << "IsaID[" << isa->getIsaID() << "] "
                     << isa->getDescription() << "\n";
    }
    
    executeIsa(isa);
    
    return !ctx.isHalted();
}

void IsaExecutor::run() {
    while (tick()) {
    }
}

void IsaExecutor::executeIsa(Isa* isa) {
    if (!isa) {
        ctx.halt();
        return;
    }
    
    // 执行指令
    isa->execute(ctx);
    
    // 根据IsaName处理控制流
    IsaName isaName = isa->getIsaName();
    
    switch (isaName) {
        case IsaName::Jump: {
            // 无条件跳转：直接跳转到目标PC
            pc = isa->getJumpTarget();
            break;
        }
        
        case IsaName::ConditionalJump: {
            // 条件跳转：根据条件值决定跳转
            auto condValue = ctx.getIntValue(isa->getCondition());
            bool cond = condValue != 0;
            if (cond) {
                pc = isa->getJumpTarget();
            } else {
                pc = isa->getFallthroughPC();
            }
            break;
        }
        
        case IsaName::Return: {
            // 返回指令
            if (!ctx.hasCallFrames()) {
                ctx.halt();
            } else {
                pc = ctx.popCallFrame();
            }
            break;
        }
        
        case IsaName::FuncCall: {
            // 函数调用：保存返回地址并跳转到函数入口
            ctx.pushCallFrame(pc + 1);
            // 实际应该查找函数入口，这里简化处理
            pc = isa->getJumpTarget();
            break;
        }
        
        case IsaName::FuncReturn: {
            // 函数返回：恢复调用者的PC
            if (!ctx.hasCallFrames()) {
                ctx.halt();
            } else {
                pc = ctx.popCallFrame();
            }
            break;
        }
        
        case IsaName::ForCondition: {
            // for循环条件判断
            auto ivValue = ctx.getIntValue(isa->getForIV());
            auto ubValue = ctx.getIntValue(isa->getForUB());
            bool cond = ivValue.slt(ubValue);  // 有符号比较
            if (cond) {
                pc = isa->getJumpTarget();  // 继续循环
            } else {
                pc = isa->getFallthroughPC();  // 退出循环
            }
            break;
        }
        
        case IsaName::IfCondition: {
            // if条件判断
            auto condValue = ctx.getIntValue(isa->getCondition());
            bool cond = condValue != 0;
            if (cond) {
                pc = isa->getJumpTarget();  // 执行then分支
            } else {
                pc = isa->getFallthroughPC();  // 执行else分支
            }
            break;
        }
        
        case IsaName::ForInit:
        case IsaName::ForIncrement:
            // for循环初始化和增量指令，PC递增
            pc++;
            break;
        
        default:
            // 普通指令，PC递增
            pc++;
            break;
    }
    
    if (isa->getKind() == Isa::Kind::Normal) {
        HardwareCharacteristics hw = isa->getHardwareCharacteristics(ctx);
        
        if (asyncMode && hw.targetUnit != ExecutionUnitType::Scalar) {
            uint64_t taskId = dispatchTask(hw.targetUnit, hw.latency, 
                                           isa->getOpName(), isa->getPC());
            if (verboseMode) {
                llvm::outs() << "  Dispatched task " << taskId << " on " 
                             << executionUnitToString(hw.targetUnit) << " unit, "
                             << "latency=" << hw.latency << "\n";
            }
        }
    }
}

uint64_t IsaExecutor::dispatchTask(ExecutionUnitType unit, uint64_t duration, 
                                    const std::string& opName, uint64_t instPC) {
    PendingTask task;
    task.taskId = nextTaskId++;
    task.startCycle = cycle;
    task.duration = duration;
    task.unit = unit;
    task.opName = opName;
    task.pc = instPC;
    
    activeTasks.push_back(task);
    
    auto& execUnit = executionUnits.at(unit);
    execUnit.dispatchTask(task);
    
    return task.taskId;
}

void IsaExecutor::updateExecutionUnits() {
    for (auto& pair : executionUnits) {
        pair.second.update(cycle);
    }
    
    activeTasks.erase(
        std::remove_if(activeTasks.begin(), activeTasks.end(),
            [this](const PendingTask& task) {
                return task.isComplete(cycle);
            }),
        activeTasks.end());
}

bool IsaExecutor::isUnitBusy(ExecutionUnitType unit) const {
    auto it = executionUnits.find(unit);
    if (it == executionUnits.end()) {
        return false;
    }
    return it->second.isBusy(cycle);
}

}