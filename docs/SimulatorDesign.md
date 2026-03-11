# MLIR仿真器设计文档

## 1. 概述

### 1.1 目标
构建一个MLIR仿真器，能够逐条执行MLIR指令，支持控制流展平、函数调用、HIVM方言、指令耗时模拟和多组件并行执行。

### 1.2 核心特性
- 基于PC（Program Counter）驱动的执行模型
- 将嵌套控制流展平为线性指令序列
- 统一的指令管理机制
- 支持多种MLIR dialect
- 指令耗时模拟（cycle级别）
- **多组件并行执行模型**（Scalar、MTE、Cube、Vec）
- **数据大小相关的延迟计算**

## 2. 系统架构

### 2.1 整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                       MLIR Simulator                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌──────────────┐   ┌──────────────────────────────┐            │
│  │    Parser    │ → │  SequenceBuilder             │            │
│  │   (现有模块)  │   │  (控制流展平 & 指令序列构建)  │            │
│  └──────────────┘   └──────────────────────────────┘            │
│         ↓                        ↓                                │
│  ┌──────────────┐   ┌──────────────────────────────┐            │
│  │ MLIR Module  │   │  InstructionSequence         │            │
│  │              │   │  (线性指令序列 + PC管理)      │            │
│  └──────────────┘   └──────────────────────────────┘            │
│                                ↓                                  │
│                    ┌──────────────────────────────┐              │
│                    │  InstructionExecutor         │              │
│                    │  (PC驱动执行引擎)             │              │
│                    └──────────────────────────────┘              │
│                                ↓                                  │
│                    ┌──────────────────────────────┐              │
│                    │  ExecutionContext            │              │
│                    │  (运行时状态 + Cycle计数)     │              │
│                    └──────────────────────────────┘              │
│                                ↓                                  │
│         ┌──────────────────────┴──────────────────────┐         │
│         │              多组件执行单元                   │         │
│         ├──────────┬──────────┬──────────┬────────────┤         │
│         │  Scalar  │   MTE    │   Cube   │    Vec     │         │
│         │ (调度)   │ (搬运)   │ (矩阵)   │  (向量)    │         │
│         └──────────┴──────────┴──────────┴────────────┘         │
│                                                                   │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 执行流程

```
1. 解析MLIR文件 → ModuleOp
2. 构建指令序列（控制流展平 + PC分配）
3. 初始化执行上下文（PC=0, cycle=0）
4. 循环：fetch instruction at PC → execute → update PC & cycle
   - Scalar指令：直接执行
   - 其他组件指令：分发任务，异步执行
5. 结束时等待所有组件完成（halt或执行完毕）
```

### 2.3 多组件执行模型

```
┌─────────────────────────────────────────────────────────────────┐
│                    Scalar (主调度单元)                           │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  PC驱动执行                                              │   │
│  │  - 标量运算：直接执行                                    │   │
│  │  - 其他指令：分发到对应组件                              │   │
│  │  - 同步点：等待组件完成                                  │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
         ↓ dispatch              ↓ dispatch           ↓ dispatch
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│       MTE       │    │      Cube       │    │       Vec       │
│  Memory Transfer│    │  Matrix Compute │    │  Vector Compute │
│                 │    │                 │    │                 │
│  - load/store   │    │  - matmul       │    │  - vadd         │
│  - 数据搬运     │    │  - 矩阵乘法     │    │  - vmul         │
│                 │    │                 │    │                 │
│  延迟 = base +  │    │  延迟 = base +  │    │  延迟 = base +  │
│  dataSize/128   │    │  dataSize/512   │    │  dataSize/256   │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## 3. 核心数据结构

### 3.1 执行单元类型

```cpp
enum class ExecutionUnitType {
    Scalar,   // 标量单元 - 调度和标量运算
    MTE,      // Memory Transfer Engine - 数据搬运
    Cube,     // 矩阵运算单元 - 矩阵乘法
    Vec       // 向量运算单元 - 向量运算
};
```

### 3.2 组件延迟模型

```cpp
struct ComponentLatencyModel {
    uint64_t baseLatency = 1;        // 基础延迟
    double bytesPerCycle = 64.0;     // 每周期处理字节数
    bool dataSizeDependent = false;  // 是否依赖数据大小
    
    uint64_t calculateLatency(size_t dataSize) const {
        if (!dataSizeDependent) {
            return baseLatency;
        }
        uint64_t dataLatency = static_cast<uint64_t>(dataSize / bytesPerCycle);
        return std::max(baseLatency, dataLatency);
    }
};
```

### 3.3 待处理任务

```cpp
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
```

### 3.4 统一指令表示

```cpp
struct Instruction {
    uint64_t pc;                        // 程序计数器
    mlir::Operation* mlirOp;            // 原始MLIR操作
    
    enum class Kind {
        Normal,                         // 普通指令
        Jump,                           // 无条件跳转
        ConditionalJump,                // 条件跳转
        ForInit,                        // 循环初始化
        ForCondition,                   // 循环条件
        ForIncrement,                   // 循环递增
        IfCondition,                    // 条件判断
        Return                          // 返回
    };
    
    Kind kind = Kind::Normal;
    
    // 跳转相关
    uint64_t jumpTarget = 0;            // 跳转目标PC
    uint64_t fallthroughPC = 0;         // fallthrough PC
    mlir::Value condition;              // 条件值
    
    // 循环相关
    mlir::Value forIV;                  // 迭代变量
    mlir::Value forLB, forUB, forStep;  // 循环边界
    std::vector<mlir::Value> iterArgs;  // 迭代参数
    std::vector<mlir::Value> iterArgInits;
    
    // 调试信息
    std::string sourceLocation;
};
```

### 3.5 指令信息（含耗时和目标单元）

```cpp
struct InstructionInfo {
    using ExecuteFunc = std::function<void(mlir::Operation*, ExecutionContext&)>;
    using DataSizeFunc = std::function<size_t(mlir::Operation*, ExecutionContext&)>;
    
    ExecuteFunc handler;
    uint64_t latency;                   // 指令耗时（cycles）
    ExecutionUnitType targetUnit;       // 目标执行单元
    bool dataSizeDependent;             // 是否依赖数据大小
    DataSizeFunc dataSizeCalculator;    // 数据大小计算函数
    ComponentLatencyModel latencyModel; // 延迟模型
    
    InstructionInfo() : latency(1), targetUnit(ExecutionUnitType::Scalar), 
                        dataSizeDependent(false) {}
    
    uint64_t calculateLatency(size_t dataSize) const {
        if (!dataSizeDependent) {
            return latency;
        }
        return latencyModel.calculateLatency(dataSize);
    }
};
```

### 3.6 指令注册表

```cpp
class InstructionRegistry {
public:
    void registerHandler(const std::string& opName, ExecuteFunc handler, 
                        uint64_t latency = 1,
                        ExecutionUnitType unit = ExecutionUnitType::Scalar);
    
    void registerHandlerWithDataSize(const std::string& opName, ExecuteFunc handler,
                                     DataSizeFunc dataSizeFunc,
                                     const ComponentLatencyModel& model,
                                     ExecutionUnitType unit);
    
    InstructionInfo* getInstructionInfo(const std::string& opName);
    bool hasHandler(const std::string& opName);
    
private:
    std::map<std::string, InstructionInfo> instructions;
};

// 注册宏
#define REGISTER_INSTRUCTION(opName, handler) \
    getGlobalRegistry().registerHandler(opName, handler)

#define REGISTER_INSTRUCTION_WITH_LATENCY(opName, handler, latency) \
    getGlobalRegistry().registerHandler(opName, handler, latency)

#define REGISTER_INSTRUCTION_ON_UNIT(opName, handler, latency, unit) \
    getGlobalRegistry().registerHandler(opName, handler, latency, unit)

#define REGISTER_INSTRUCTION_WITH_DATA_SIZE(opName, handler, dataSizeFunc, model, unit) \
    getGlobalRegistry().registerHandlerWithDataSize(opName, handler, dataSizeFunc, model, unit)
```

### 3.7 执行上下文（含Cycle计数和多组件管理）

```cpp
class ExecutionContext {
    // 值存储
    std::map<void*, llvm::APInt> intValues;
    std::map<void*, llvm::APFloat> floatValues;
    std::map<void*, std::vector<uint8_t>> memrefData;
    
    // 执行状态
    uint64_t pc;                          // 当前程序计数器
    uint64_t cycle;                       // 当前cycle
    bool halted;                          // 是否停止
    
    // 调用栈
    struct CallFrame {
        uint64_t returnPC;
        std::map<void*, llvm::APInt> savedIntValues;
        std::map<void*, llvm::APFloat> savedFloatValues;
    };
    std::vector<CallFrame> callStack;
    
    // 多组件执行支持
    std::map<ExecutionUnitType, ExecutionUnit> executionUnits;
    std::vector<PendingTask> activeTasks;
    uint64_t nextTaskId;
    
public:
    // 值操作
    void setIntValue(mlir::Value v, llvm::APInt val);
    void setFloatValue(mlir::Value v, llvm::APFloat val);
    void setMemrefData(mlir::Value v, std::vector<uint8_t> data);
    
    // PC管理
    void setPC(uint64_t newPC) { pc = newPC; }
    uint64_t getPC() const { return pc; }
    void nextPC() { pc++; }
    
    // Cycle管理
    uint64_t getCycle() const { return cycle; }
    void advanceCycle(uint64_t cycles) { cycle += cycles; }
    void setCycle(uint64_t c) { cycle = c; }
    
    // 执行控制
    void halt() { halted = true; }
    bool isHalted() const { return halted; }
    
    // 调用栈管理
    void pushCallFrame(uint64_t returnPC);
    uint64_t popCallFrame();
    
    // 多组件执行管理
    uint64_t dispatchTask(ExecutionUnitType unit, uint64_t duration, 
                          const std::string& opName, uint64_t pc);
    void updateExecutionUnits();
    bool isUnitBusy(ExecutionUnitType unit) const;
    uint64_t getUnitBusyUntil(ExecutionUnitType unit) const;
    bool allUnitsIdle() const;
    uint64_t getEarliestCompletionCycle() const;
    void advanceToCompletion(ExecutionUnitType unit);
};
```

### 3.8 指令执行器

```cpp
class InstructionExecutor {
    InstructionSequence sequence;
    ExecutionContext ctx;
    bool verboseMode;
    bool asyncMode;  // 异步模式开关
    
public:
    void load(mlir::ModuleOp module);
    bool executeNext();
    void run();
    
    uint64_t getCurrentPC() const { return ctx.getPC(); }
    bool isHalted() const { return ctx.isHalted(); }
    
    ExecutionContext& getContext() { return ctx; }
    InstructionSequence& getSequence() { return sequence; }
    
    void setVerbose(bool verbose) { verboseMode = verbose; }
    void setAsyncMode(bool async) { asyncMode = async; }
    
private:
    void executeInstruction(Instruction* inst);
    ExecutionResult executeOp(mlir::Operation* op);
    uint64_t executeNormalInstruction(Instruction* inst);
    void executeJumpInstruction(Instruction* inst);
    void executeConditionalJumpInstruction(Instruction* inst);
    void executeForInitInstruction(Instruction* inst);
    void executeForConditionInstruction(Instruction* inst);
    void executeForIncrementInstruction(Instruction* inst);
    void executeReturnInstruction();
    
    void waitForAllUnits();
    void waitForUnit(ExecutionUnitType unit);
};
```

## 4. 指令耗时模拟

### 4.1 多组件延迟配置

每个组件有独立的延迟模型，延迟与数据大小相关：

| 组件 | 基础延迟 | 吞吐量 | 指令 |
|------|----------|--------|------|
| Scalar | 1-12 cycles | N/A | arith运算 |
| MTE | 10 cycles | 128 bytes/cycle | load, store |
| Vec | 5 cycles | 256 bytes/cycle | vadd, vmul |
| Cube | 20 cycles | 512 bytes/cycle | matmul |

### 4.2 数据大小相关延迟计算

```cpp
// 延迟 = max(baseLatency, dataSize / bytesPerCycle)

// 示例：memref<128xf16> 的 load 操作
// dataSize = 128 * 2 = 256 bytes
// latency = max(10, 256 / 128) = max(10, 2) = 10 cycles

// 示例：memref<64x64xf16> 的 matmul 操作
// dataSize = 64*64*2 + 64*64*2 = 16384 bytes
// latency = max(20, 16384 / 512) = max(20, 32) = 32 cycles
```

### 4.3 固定延迟指令

| 指令 | 耗时 (cycles) | 说明 |
|------|---------------|------|
| memref.alloc | 1 | 内存分配 |
| hivm.hir.load | 100 | GM到UB数据加载 |
| hivm.hir.vadd | 50 | 向量加法 |
| hivm.hir.store | 100 | UB到GM数据存储 |
| arith.addi/subi | 1 | 整数加减 |
| arith.muli | 3 | 整数乘法 |
| arith.divsi | 10 | 整数除法 |
| arith.addf/subf | 2 | 浮点加减 |
| arith.mulf | 4 | 浮点乘法 |
| arith.divf | 12 | 浮点除法 |
| func.call | 5 | 函数调用 |
| func.return | 1 | 函数返回 |

### 4.4 执行流程

```cpp
void InstructionExecutor::executeInstruction(Instruction* inst) {
    uint64_t latency = 1;
    
    switch (inst->kind) {
        case Instruction::Kind::Normal:
            latency = executeNormalInstruction(inst);
            break;
        // ... other cases
    }
    
    ctx.advanceCycle(latency);
}

uint64_t InstructionExecutor::executeNormalInstruction(Instruction* inst) {
    if (inst->mlirOp) {
        ExecutionResult result = executeOp(inst->mlirOp);
        
        // 非Scalar组件且启用异步模式
        if (result.targetUnit != ExecutionUnitType::Scalar && asyncMode) {
            // 分发任务到对应组件
            ctx.dispatchTask(result.targetUnit, result.latency, 
                            inst->mlirOp->getName().getStringRef().str(),
                            inst->pc);
            advancePC();
            return 1;  // Scalar只消耗1个cycle
        }
        
        advancePC();
        return result.latency;
    }
    advancePC();
    return 1;
}

ExecutionResult InstructionExecutor::executeOp(mlir::Operation* op) {
    std::string opName = op->getName().getStringRef().str();
    
    auto& registry = getGlobalRegistry();
    if (registry.hasHandler(opName)) {
        auto* info = registry.getInstructionInfo(opName);
        if (info && info->handler) {
            info->handler(op, ctx);
            
            ExecutionResult result;
            result.targetUnit = info->targetUnit;
            
            if (info->dataSizeDependent && info->dataSizeCalculator) {
                result.dataSize = info->dataSizeCalculator(op, ctx);
                result.latency = info->calculateLatency(result.dataSize);
            } else {
                result.latency = info->latency;
            }
            
            return result;
        }
    }
    return ExecutionResult();
}

void InstructionExecutor::waitForAllUnits() {
    while (!ctx.allUnitsIdle()) {
        uint64_t nextCompletion = ctx.getEarliestCompletionCycle();
        ctx.setCycle(nextCompletion);
        ctx.updateExecutionUnits();
    }
}
```

## 5. 控制流展平策略

### 5.1 scf.for 展平规则

```
原始MLIR:
  scf.for %i = %lb to %ub step %step iter_args(%sum = %init) {
    %new_sum = arith.addi %sum, %i
    scf.yield %new_sum
  }

展平后的指令序列:
  PC  | Kind        | Description
  ----|-------------|----------------------------------------
  0   | ForInit     | %i = %lb, %sum = %init
  1   | Jump        | → PC=2
  2   | ForCondition| if (%i < %ub) → PC=4 else → PC=8
  3   | (unused)
  4   | Normal      | %new_sum = arith.addi %sum, %i
  5   | Normal      | %sum = %new_sum (yield assignment)
  6   | ForIncrement| %i = %i + %step
  7   | Jump        | → PC=2
  8   | (continue...)
```

### 5.2 scf.if 展平规则

```
原始MLIR:
  scf.if %cond {
    %a = arith.addi %x, %y
  } else {
    %b = arith.subi %x, %y
  }

展平后的指令序列:
  PC  | Kind        | Description
  ----|-------------|----------------------------------------
  0   | IfCondition | if (%cond) → PC=2 else → PC=5
  1   | (unused)
  2   | Normal      | %a = arith.addi %x, %y
  3   | Jump        | → PC=7
  4   | (unused)
  5   | Normal      | %b = arith.subi %x, %y
  6   | Jump        | → PC=7
  7   | (continue...)
```

## 6. 支持的MLIR操作

### 6.1 arith dialect
- `arith.constant` - 常量
- `arith.addi/subi/muli/divsi` - 整数运算
- `arith.addf/subf/mulf/divf` - 浮点运算
- `arith.cmpi` - 整数比较
- `arith.index_cast` - 类型转换

### 6.2 func dialect
- `func.func` - 函数定义
- `func.call` - 函数调用
- `func.return` - 函数返回

### 6.3 scf dialect
- `scf.for` - for循环
- `scf.if` - 条件分支
- `scf.yield` - 产生值

### 6.4 memref dialect
- `memref.alloc` - 内存分配

### 6.5 hivm dialect
- `hivm.hir.load` - GM到UB数据加载（MTE组件）
- `hivm.hir.vadd` - 向量加法（Vec组件）
- `hivm.hir.vmul` - 向量乘法（Vec组件）
- `hivm.hir.matmul` - 矩阵乘法（Cube组件）
- `hivm.hir.store` - UB到GM数据存储（MTE组件）

## 7. 文件结构

```
ascendir_parser/
├── include/
│   ├── Instruction.h               # 指令表示
│   ├── InstructionSequence.h       # 指令序列
│   ├── InstructionRegistry.h       # 指令注册表（含耗时和目标单元）
│   ├── ExecutionContext.h          # 执行上下文（含cycle和多组件管理）
│   ├── ExecutionUnit.h             # 执行单元定义（MTE/Cube/Vec）
│   └── InstructionExecutor.h       # 指令执行器
│
├── src/
│   ├── InstructionSequence.cpp     # 序列构建和管理
│   ├── ExecutionContext.cpp        # 运行时状态管理
│   ├── InstructionRegistry.cpp     # 注册表实现
│   ├── InstructionExecutor.cpp     # PC驱动执行引擎
│   ├── Instructions/
│   │   ├── ArithInstructions.cpp   # arith操作
│   │   ├── FuncInstructions.cpp    # func操作
│   │   └── HivmInstructions.cpp    # hivm操作（含多组件支持）
│   └── main.cpp                    # 主程序
│
└── test/
    ├── test_simple.mlir            # 简单测试
    ├── test_for.mlir               # 循环测试
    ├── test_if.mlir                # 条件分支测试
    ├── test_call.mlir              # 函数调用测试
    ├── test_hivm_basic.mlir        # HIVM方言测试
    ├── test_multi_component.mlir   # 多组件协作测试
    ├── test_matmul_pipeline.mlir   # 矩阵乘法流水线测试
    └── test_pipeline.mlir          # 完整流水线测试
```

## 8. 扩展机制

### 8.1 添加新指令

```cpp
// 1. 实现处理器函数
void executeMyCustomOp(mlir::Operation* op, ExecutionContext& ctx) {
    auto myOp = llvm::cast<MyDialect::MyOp>(op);
    // 执行逻辑
}

// 2. 注册指令（固定延迟，Scalar组件）
REGISTER_INSTRUCTION_WITH_LATENCY("mydialect.myop", executeMyCustomOp, 10);

// 3. 注册指令到特定组件
REGISTER_INSTRUCTION_ON_UNIT("mydialect.vecop", executeVecOp, 5, ExecutionUnitType::Vec);

// 4. 注册数据大小相关延迟的指令
ComponentLatencyModel model;
model.baseLatency = 10;
model.bytesPerCycle = 256.0;
model.dataSizeDependent = true;

REGISTER_INSTRUCTION_WITH_DATA_SIZE("mydialect.dataop", executeDataOp,
                                    calculateDataSize, model,
                                    ExecutionUnitType::MTE);
```

### 8.2 添加新Dialect

```cpp
void registerMyDialect() {
    // 固定延迟指令
    REGISTER_INSTRUCTION_WITH_LATENCY("mydialect.op1", executeOp1, 5);
    
    // 多组件指令
    REGISTER_INSTRUCTION_ON_UNIT("mydialect.vecop", executeVecOp, 5, ExecutionUnitType::Vec);
    
    // 数据大小相关延迟
    ComponentLatencyModel model;
    model.baseLatency = 10;
    model.bytesPerCycle = 128.0;
    model.dataSizeDependent = true;
    
    REGISTER_INSTRUCTION_WITH_DATA_SIZE("mydialect.mteop", executeMteOp,
                                        calculateDataSize, model,
                                        ExecutionUnitType::MTE);
}
```

## 9. 输出格式

### 9.1 异步模式详细输出

```
Starting simulation (mode: asynchronous)...

[cycle=0] PC 0: Normal: %ubA = memref.alloc() : memref<128xf16>
  [memref.alloc] Allocated 256 bytes for memref<128xf16>
[cycle=1] Scalar advanced 1 cycles

[cycle=1] PC 1: Normal: "hivm.hir.load"(%valueA, %ubA) : ...
  [hivm.hir.load] GM -> UB on MTE
    Data size: 256 bytes (128 elements)
    Initialized 128 elements (test data): [0, 1, 2, 3, ..., 126, 127]
  [Dispatch] Task to MTE, duration=10 cycles, dataSize=256 bytes
  Active tasks:
    - hivm.hir.load on MTE (complete at cycle 11)

[cycle=2] PC 2: Normal: %ubB = memref.alloc() : memref<128xf16>
  [memref.alloc] Allocated 256 bytes for memref<128xf16>

[cycle=3] PC 3: Normal: "hivm.hir.load"(%valueB, %ubB) : ...
  [hivm.hir.load] GM -> UB on MTE
    Data size: 256 bytes (128 elements)
  [Dispatch] Task to MTE, duration=10 cycles, dataSize=256 bytes
  Active tasks:
    - hivm.hir.load on MTE (complete at cycle 11)
    - hivm.hir.load on MTE (complete at cycle 13)

...

Simulation completed.
Final PC: 12
Total cycles: 56
All units idle: yes
```

### 9.2 同步模式对比

```
Starting simulation (mode: synchronous)...

[cycle=0] PC 0: Normal: %ubA = memref.alloc() : memref<128xf16>
  [memref.alloc] Allocated 256 bytes for memref<128xf16>
[cycle=1] Scalar advanced 1 cycles

[cycle=1] PC 1: Normal: "hivm.hir.load"(%valueA, %ubA) : ...
  [hivm.hir.load] GM -> UB on MTE
    Data size: 256 bytes (128 elements)
[cycle=11] Scalar advanced 10 cycles

...

Simulation completed.
Final PC: 12
Total cycles: 326
All units idle: yes
```

### 9.3 格式说明

- `[cycle=X]` - 当前cycle数
- `PC Y` - 当前程序计数器
- `Normal/Jump/ForCondition...` - 指令类型
- `[Dispatch] Task to <unit>` - 任务分发到对应组件
- `Active tasks:` - 当前活跃的后台任务
- `complete at cycle X` - 任务完成时间

## 10. 性能优化方向

### 10.1 指令缓存
- 缓存频繁执行的指令序列
- 预取接下来的N条指令

### 10.2 JIT编译
- 将热点指令序列编译为机器码
- 使用LLVM JIT

### 10.3 多组件并行执行（已实现）
- Scalar分发任务后继续执行
- MTE、Vec、Cube组件并行工作
- 数据大小相关延迟计算
- 异步任务管理和同步等待

### 10.4 流水线模拟（部分实现）
- 指令级并行
- 组件级并行
- 待实现：乱序执行模拟

### 10.5 未来优化
- 依赖分析：自动识别无依赖的指令
- 任务调度优化：智能分配组件任务
- 内存访问优化：预取和缓存

## 11. 总结

本设计提供了一个完整的MLIR仿真器框架，具有以下特点：

1. **PC驱动**：简单清晰的执行模型
2. **线性指令序列**：避免嵌套执行，支持指令级调度
3. **统一指令管理**：注册表模式，易于扩展
4. **模块化设计**：各组件职责明确，易于维护
5. **指令耗时模拟**：支持cycle级别的性能分析
6. **控制流展平**：支持scf.for和scf.if展平
7. **多组件并行执行**：支持Scalar、MTE、Cube、Vec组件并行工作
8. **数据大小相关延迟**：根据数据量动态计算延迟

该设计为后续的指令发射优化提供了良好的基础框架，支持真实的NPU硬件行为模拟。
