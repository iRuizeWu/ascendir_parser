#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <map>
#include <queue>
#include <functional>

namespace ascendir_parser {

enum class ExecutionUnitType {
    Scalar,
    MTE,
    Cube,
    Vec
};

inline std::string executionUnitToString(ExecutionUnitType unit) {
    switch (unit) {
        case ExecutionUnitType::Scalar: return "Scalar";
        case ExecutionUnitType::MTE: return "MTE";
        case ExecutionUnitType::Cube: return "Cube";
        case ExecutionUnitType::Vec: return "Vec";
    }
    return "Unknown";
}

struct PendingTask {
    uint64_t taskId;
    uint64_t startCycle;
    uint64_t duration;
    ExecutionUnitType unit;
    std::string opName;
    uint64_t pc;
    
    bool isComplete(uint64_t currentCycle) const {
        return currentCycle >= startCycle + duration;
    }
    
    uint64_t completeCycle() const {
        return startCycle + duration;
    }
};

class ExecutionUnit {
    ExecutionUnitType type;
    bool busy;
    uint64_t busyUntilCycle;
    std::queue<PendingTask> pendingTasks;
    
public:
    ExecutionUnit(ExecutionUnitType t) : type(t), busy(false), busyUntilCycle(0) {}
    
    bool isBusy(uint64_t currentCycle) const {
        return currentCycle < busyUntilCycle;
    }
    
    uint64_t getBusyUntilCycle() const {
        return busyUntilCycle;
    }
    
    void dispatchTask(const PendingTask& task) {
        pendingTasks.push(task);
        busyUntilCycle = task.startCycle + task.duration;
        busy = true;
    }
    
    void update(uint64_t currentCycle) {
        while (!pendingTasks.empty()) {
            if (pendingTasks.front().isComplete(currentCycle)) {
                pendingTasks.pop();
            } else {
                break;
            }
        }
        busy = isBusy(currentCycle);
    }
    
    ExecutionUnitType getType() const { return type; }
};

struct ComponentLatencyModel {
    uint64_t baseLatency = 1;
    double bytesPerCycle = 64.0;
    bool dataSizeDependent = false;
    
    uint64_t calculateLatency(size_t dataSize) const {
        if (!dataSizeDependent) {
            return baseLatency;
        }
        if (bytesPerCycle <= 0) {
            return baseLatency;
        }
        uint64_t dataLatency = static_cast<uint64_t>(dataSize / bytesPerCycle);
        return std::max(baseLatency, dataLatency);
    }
};

}
