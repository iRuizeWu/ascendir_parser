#pragma once

#include "ExecutionContext.h"
#include "ExecutionUnit.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Value.h"
#include <cstdint>
#include <string>
#include <memory>

namespace ascendir_parser {

using IsaID = uint64_t;

// IsaName 枚举：标识不同的指令类型
enum class IsaName {
    // Arith operations
    ArithConstant,
    ArithAddi,
    ArithSubi,
    ArithMuli,
    ArithDivi,
    ArithAddf,
    ArithSubf,
    ArithMulf,
    ArithDivf,
    ArithCmpi,
    ArithIndexCast,
    
    // Control flow operations
    Jump,
    ConditionalJump,
    Return,
    ForInit,
    ForCondition,
    ForIncrement,
    IfCondition,
    YieldAssign,
    IfYield,
    
    // Func operations
    FuncCall,
    FuncReturn,
    
    // Memref operations
    MemrefAlloc,
    
    // HIVM operations
    HivmHirLoad,
    HivmHirStore,
    HivmHirVadd,
    HivmHirVmul,
    HivmHirMatmul,
    
    // Unknown
    Unknown
};

struct HardwareCharacteristics {
    uint64_t latency;
    ExecutionUnitType targetUnit;
    size_t dataSize;
    
    HardwareCharacteristics() : latency(1), targetUnit(ExecutionUnitType::Scalar), dataSize(0) {}
};

class Isa {
protected:
    IsaID isaID;
    uint64_t pc;
    mlir::Operation* mlirOp;
    std::string opName;
    IsaName isaName;  // 新增：指令名称标识
    
    static IsaID nextIsaID;
    
public:
    enum class Kind {
        Normal,
        Jump,
        ConditionalJump,
        Return,
        ForInit,
        ForCondition,
        ForIncrement,
        IfCondition
    };
    
protected:
    Kind kind = Kind::Normal;
    
    uint64_t jumpTarget = 0;
    uint64_t fallthroughPC = 0;
    mlir::Value condition;
    
    std::string sourceLocation;
    
    mlir::Value forIV;
    mlir::Value forLB;
    mlir::Value forUB;
    mlir::Value forStep;
    std::vector<mlir::Value> iterArgs;
    std::vector<mlir::Value> iterArgInits;
    mlir::Operation* forOp = nullptr;
    
public:
    Isa() : isaID(nextIsaID++), pc(0), mlirOp(nullptr), isaName(IsaName::Unknown), kind(Kind::Normal) {}
    virtual ~Isa() = default;
    
    IsaID getIsaID() const { return isaID; }
    uint64_t getPC() const { return pc; }
    void setPC(uint64_t p) { pc = p; }
    
    mlir::Operation* getMlirOp() const { return mlirOp; }
    void setMlirOp(mlir::Operation* op) { mlirOp = op; }
    
    IsaName getIsaName() const { return isaName; }
    void setIsaName(IsaName name) { isaName = name; }
    
    Kind getKind() const { return kind; }
    void setKind(Kind k) { kind = k; }
    
    const std::string& getOpName() const { return opName; }
    void setOpName(const std::string& name) { opName = name; }
    
    uint64_t getJumpTarget() const { return jumpTarget; }
    void setJumpTarget(uint64_t target) { jumpTarget = target; }
    
    uint64_t getFallthroughPC() const { return fallthroughPC; }
    void setFallthroughPC(uint64_t pc) { fallthroughPC = pc; }
    
    mlir::Value getCondition() const { return condition; }
    void setCondition(mlir::Value cond) { condition = cond; }
    
    virtual bool decode(mlir::Operation* op) {
        mlirOp = op;
        if (op) {
            opName = op->getName().getStringRef().str();
        }
        return true;
    }
    
    // 修改：返回值改为 void
    virtual void execute(ExecutionContext& ctx) = 0;
    
    virtual ExecutionUnitType getExecutionUnit() const {
        return ExecutionUnitType::Scalar;
    }
    
    virtual uint64_t getLatency() const {
        return 1;
    }
    
    virtual HardwareCharacteristics getHardwareCharacteristics(ExecutionContext& ctx) {
        HardwareCharacteristics hw;
        hw.latency = getLatency();
        hw.targetUnit = getExecutionUnit();
        return hw;
    }
    
    virtual std::string getDescription() const {
        return opName.empty() ? "Unknown" : opName;
    }
    
    static void resetIsaID() { nextIsaID = 0; }
    static IsaID getCurrentIsaID() { return nextIsaID; }
    
    mlir::Value getForIV() const { return forIV; }
    void setForIV(mlir::Value v) { forIV = v; }
    
    mlir::Value getForLB() const { return forLB; }
    void setForLB(mlir::Value v) { forLB = v; }
    
    mlir::Value getForUB() const { return forUB; }
    void setForUB(mlir::Value v) { forUB = v; }
    
    mlir::Value getForStep() const { return forStep; }
    void setForStep(mlir::Value v) { forStep = v; }
    
    const std::vector<mlir::Value>& getIterArgs() const { return iterArgs; }
    void setIterArgs(const std::vector<mlir::Value>& args) { iterArgs = args; }
    
    const std::vector<mlir::Value>& getIterArgInits() const { return iterArgInits; }
    void setIterArgInits(const std::vector<mlir::Value>& inits) { iterArgInits = inits; }
    
    mlir::Operation* getForOp() const { return forOp; }
    void setForOp(mlir::Operation* op) { forOp = op; }
    
    const std::string& getSourceLocation() const { return sourceLocation; }
    void setSourceLocation(const std::string& loc) { sourceLocation = loc; }
};

using IsaPtr = std::unique_ptr<Isa>;

}