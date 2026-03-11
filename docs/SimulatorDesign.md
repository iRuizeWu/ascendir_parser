# MLIR仿真器设计文档

## 1. 概述

### 1.1 目标
构建一个MLIR仿真器，能够逐条执行MLIR指令，支持控制流展平、函数调用、HIVM方言、指令耗时模拟和多组件并行执行。

### 1.2 核心特性
- 基于 PC（Program Counter）驱动的执行模型
- 将嵌套控制流展平为线性指令序列
- 统一的指令管理机制
- 支持多种 MLIR dialect
- 指令耗时模拟（cycle 级别）
- **多组件并行执行模型**（Scalar、MTE、Cube、Vec）
- **数据大小相关的延迟计算**
- **Isa 类自描述硬件特性**（执行单元、延迟）

## 2. 系统架构

### 2.1 整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                       MLIR Simulator                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌──────────────┐   ┌──────────────────────────────┐            │
│  │    Parser    │ → │  IsaSequenceBuilder          │            │
│  │   (现有模块)  │   │  (控制流展平 & 指令序列构建)  │            │
│  └──────────────┘   └──────────────────────────────┘            │
│         ↓                        ↓                                │
│  ┌──────────────┐   ┌──────────────────────────────┐            │
│  │ MLIR Module  │   │  IsaSequence                 │            │
│  │              │   │  (线性指令序列 + PC管理)      │            │
│  └──────────────┘   └──────────────────────────────┘            │
│                                ↓                                  │
│                    ┌──────────────────────────────┐              │
│                    │  IsaExecutor                 │              │
│                    │  (PC驱动执行引擎)             │              │
│                    │  - PC 管理                   │              │
│                    │  - Cycle 管理                │              │
│                    │  - 执行单元实例化             │              │
│                    │  - 阻塞状态管理               │              │
│                    └──────────────────────────────┘              │
│                                ↓                                  │
│                    ┌──────────────────────────────┐              │
│                    │  ExecutionContext            │              │
│                    │  (运行时数据存储)             │              │
│                    │  - 值存储                    │              │
│                    │  - 调用栈管理                 │              │
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
1. 解析 MLIR 文件 → ModuleOp
2. 构建指令序列（控制流展平 + PC 分配）
3. 初始化 IsaExecutor（PC=0, cycle=0）
4. 循环 tick()：
   - cycle++
   - 更新执行单元状态
   - 检查阻塞状态
   - 获取 PC 处的 Isa
   - 执行 Isa::execute(ctx) → IsaExecuteResult
   - 根据 result 更新 PC
   - 获取 Isa::getHardwareCharacteristics(ctx) → 分发任务
5. 结束（halt 或执行完毕）
```

### 2.3 多组件执行模型

```
┌─────────────────────────────────────────────────────────────────┐
│                    IsaExecutor (主调度器)                        │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  tick() 驱动执行                                         │   │
│  │  - PC 管理：决定下一条执行的指令                          │   │
│  │  - Cycle 管理：时间推进                                  │   │
│  │  - 阻塞管理：等待执行单元完成                             │   │
│  │  - 任务分发：根据 Isa 的硬件特性分发任务                   │   │
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
│  延迟由 Isa     │    │  延迟由 Isa     │    │  延迟由 Isa     │
│  类自描述       │    │  类自描述       │    │  类自描述       │
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

### 3.2 硬件特性结构体

```cpp
struct HardwareCharacteristics {
    uint64_t latency;           // 延迟：任务完成所需时间
    ExecutionUnitType targetUnit; // 目标执行单元
    size_t dataSize;            // 数据大小（用于动态延迟计算）
    
    HardwareCharacteristics() 
        : latency(1), targetUnit(ExecutionUnitType::Scalar), dataSize(0) {}
};
```

### 3.3 执行结果结构体

```cpp
struct IsaExecuteResult {
    enum class Action {
        Continue,   // pc++
        Jump,       // pc = target
        Halt,       // 停止执行
        Call,       // 函数调用
        Return      // 函数返回
    };
    
    Action action = Action::Continue;
    uint64_t jumpTarget = 0;
    uint64_t returnPC = 0;
    
    static IsaExecuteResult continueExecution();
    static IsaExecuteResult jumpTo(uint64_t target);
    static IsaExecuteResult halt();
    static IsaExecuteResult call(uint64_t target, uint64_t retPC);
    static IsaExecuteResult ret();
};
```

### 3.4 Isa 基类（自描述硬件特性）

```cpp
class Isa {
protected:
    IsaID isaID;
    uint64_t pc;
    mlir::Operation* mlirOp;
    std::string opName;
    Kind kind = Kind::Normal;
    
    // 跳转和循环相关字段
    uint64_t jumpTarget = 0;
    uint64_t fallthroughPC = 0;
    mlir::Value condition;
    // ...
    
public:
    enum class Kind {
        Normal, ConditionalJump, Jump, Return,
        ForInit, ForCondition, ForIncrement, IfCondition
    };
    
    // 核心虚方法
    virtual IsaExecuteResult execute(ExecutionContext& ctx) = 0;
    
    // 硬件特性描述（子类重写）
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
    
    virtual std::string getDescription() const;
};
```

### 3.5 待处理任务

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

### 3.6 ExecutionContext（简化版）

```cpp
class ExecutionContext {
    // 值存储
    std::map<void*, llvm::APInt> intValues;
    std::map<void*, llvm::APFloat> floatValues;
    std::map<void*, std::vector<uint8_t>> memrefData;
    
    // 调用栈
    struct CallFrame {
        uint64_t returnPC;
        std::map<void*, llvm::APInt> savedIntValues;
        std::map<void*, llvm::APFloat> savedFloatValues;
    };
    std::vector<CallFrame> callStack;
    
    // 执行状态
    bool halted;
    
public:
    // 值操作
    void setIntValue(mlir::Value v, llvm::APInt val);
    void setFloatValue(mlir::Value v, llvm::APFloat val);
    void setMemrefData(mlir::Value v, std::vector<uint8_t> data);
    
    // 执行控制
    void halt() { halted = true; }
    bool isHalted() const { return halted; }
    void clearHalt() { halted = false; }
    
    // 调用栈管理
    void pushCallFrame(uint64_t returnPC);
    uint64_t popCallFrame();
    bool hasCallFrames() const { return !callStack.empty(); }
};
```

### 3.7 IsaExecutor（核心调度器）

```cpp
class IsaExecutor {
    IsaSequence sequence;
    ExecutionContext ctx;
    
    // PC 和 Cycle 管理（从 ExecutionContext 移入）
    uint64_t pc;
    uint64_t cycle;
    
    // 执行单元实例化
    std::map<ExecutionUnitType, ExecutionUnit> executionUnits;
    std::vector<PendingTask> activeTasks;
    uint64_t nextTaskId;
    
    // 执行模式
    bool verboseMode;
    bool asyncMode;
    
    // 阻塞状态
    bool blocked;
    ExecutionUnitType blockingUnit;
    
public:
    void load(mlir::ModuleOp module);
    bool tick();                    // 一次时钟周期操作
    void run();                     // 完整执行
    
    uint64_t getCurrentPC() const { return pc; }
    uint64_t getCurrentCycle() const { return cycle; }
    bool isHalted() const { return ctx.isHalted(); }
    bool isBlocked() const { return blocked; }
    
    void setVerbose(bool verbose);
    void setAsyncMode(bool async);
    
private:
    void executeIsa(Isa* isa);
    uint64_t dispatchTask(ExecutionUnitType unit, uint64_t duration, 
                          const std::string& opName, uint64_t instPC);
    void updateExecutionUnits();
};
```

### 3.8 IsaRegistry（简化版）

```cpp
class IsaRegistry {
public:
    using IsaFactory = std::function<IsaPtr()>;
    
    void registerIsa(const std::string& opName, IsaFactory factory);
    IsaPtr createIsa(const std::string& opName);
    bool hasIsa(const std::string& opName);
    
    static IsaRegistry& getGlobalRegistry();
};

// 注册宏（简化版）
#define REGISTER_ISA(opName, IsaClass) \
    IsaRegistry::getGlobalRegistry().registerIsa(opName, \
        []() -> IsaPtr { return std::make_unique<IsaClass>(); })
```

## 4. 指令耗时模拟

### 4.1 Isa 类自描述硬件特性

每个 Isa 子类直接定义自己的硬件特性：

```cpp
class HivmHirLoadIsa : public Isa {
public:
    // 执行单元
    ExecutionUnitType getExecutionUnit() const override {
        return ExecutionUnitType::MTE;
    }
    
    // 基础延迟
    uint64_t getLatency() const override {
        return 10;
    }
    
    // 动态延迟计算（根据数据大小）
    HardwareCharacteristics getHardwareCharacteristics(ExecutionContext& ctx) override {
        HardwareCharacteristics hw;
        hw.targetUnit = ExecutionUnitType::MTE;
        
        if (mlirOp && mlirOp->getNumOperands() >= 2) {
            hw.dataSize = getMemrefSize(mlirOp->getOperand(1));
            hw.latency = 5 + hw.dataSize / 1024;  // 动态计算
        } else {
            hw.latency = 10;
        }
        return hw;
    }
    
    IsaExecuteResult execute(ExecutionContext& ctx) override {
        // 执行逻辑...
        return IsaExecuteResult::continueExecution();
    }
};
```

### 4.2 组件延迟配置

| 组件 | 基础延迟 | 吞吐量 | 指令 |
|------|----------|--------|------|
| Scalar | 1-12 cycles | N/A | arith 运算 |
| MTE | 10 cycles | 128 bytes/cycle | load, store |
| Vec | 5 cycles | 256 bytes/cycle | vadd, vmul |
| Cube | 20 cycles | 512 bytes/cycle | matmul |

### 4.3 延迟、吞吐、Cycle 数的关系

```
┌─────────────────────────────────────────────────────────────┐
│                    执行单元流水线示例                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  Cycle:  1    2    3    4    5    6    7    8    9   10    │
│         ┌────┬────┬────┬────┬────┬────┬────┬────┬────┬────┐ │
│  Task1  │ F1 │ D1 │ E1 │ E1 │ E1 │ W1 │    │    │    │    │ │
│         ├────┼────┼────┼────┼────┼────┼────┼────┼────┼────┤ │
│  Task2  │    │ F2 │ D2 │ E2 │ E2 │ E2 │ W2 │    │    │    │ │
│         ├────┼────┼────┼────┼────┼────┼────┼────┼────┼────┤ │
│  Task3  │    │    │ F3 │ D3 │ E3 │ E3 │ E3 │ W3 │    │    │ │
│         └────┴────┴────┴────┴────┴────┴────┴────┴────┴────┘ │
│                                                             │
│  延迟 = 5 cycles (单个任务从开始到完成)                        │
│  吞吐 = 1 task/cycle (每个 cycle 完成一个任务)                │
│  Cycle 数 = 5 cycles (执行阶段占用)                          │
└─────────────────────────────────────────────────────────────┘

关键公式：
  吞吐 = 1 / (完成一个操作的平均时间)
  
  如果流水线深度 = D，每级延迟 = L：
    总延迟 = D × L
    理想吞吐 = 1 / L  (每级完成后可接受新任务)
```

### 4.4 执行流程

```cpp
void IsaExecutor::executeIsa(Isa* isa) {
    // 1. 执行指令语义（功能正确性）
    IsaExecuteResult result = isa->execute(ctx);
    
    // 2. 根据 result 更新 PC
    switch (result.action) {
        case IsaExecuteResult::Action::Continue:
            pc++;
            break;
        case IsaExecuteResult::Action::Jump:
            pc = result.jumpTarget;
            break;
        // ...
    }
    
    // 3. 获取硬件特性并分发任务（性能模拟）
    if (isa->getKind() == Isa::Kind::Normal) {
        HardwareCharacteristics hw = isa->getHardwareCharacteristics(ctx);
        
        if (asyncMode && hw.targetUnit != ExecutionUnitType::Scalar) {
            dispatchTask(hw.targetUnit, hw.latency, isa->getOpName(), isa->getPC());
        }
    }
}
```

## 5. 控制流展平策略

### 5.1 scf.for 展平规则

```
原始 MLIR:
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
  5   | YieldAssign | %sum = %new_sum (yield assignment)
  6   | ForIncrement| %i = %i + %step
  7   | Jump        | → PC=2
  8   | (continue...)
```

### 5.2 scf.if 展平规则

```
原始 MLIR:
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

## 6. 支持的 MLIR 操作

### 6.1 arith dialect
- `arith.constant` - 常量 (1 cycle, Scalar)
- `arith.addi/subi` - 整数加减 (1 cycle, Scalar)
- `arith.muli` - 整数乘法 (3 cycles, Scalar)
- `arith.divsi` - 整数除法 (10 cycles, Scalar)
- `arith.addf/subf` - 浮点加减 (2 cycles, Scalar)
- `arith.mulf` - 浮点乘法 (4 cycles, Scalar)
- `arith.divf` - 浮点除法 (12 cycles, Scalar)
- `arith.cmpi` - 整数比较 (1 cycle, Scalar)
- `arith.index_cast` - 类型转换 (5 cycles, Scalar)

### 6.2 func dialect
- `func.call` - 函数调用 (5 cycles, Scalar)
- `func.return` - 函数返回 (1 cycle, Scalar)

### 6.3 scf dialect
- `scf.for` - for 循环
- `scf.if` - 条件分支
- `scf.yield` - 产生值

### 6.4 memref dialect
- `memref.alloc` - 内存分配 (1 cycle, Scalar)

### 6.5 hivm dialect
- `hivm.hir.load` - GM 到 UB 数据加载 (MTE, 动态延迟)
- `hivm.hir.vadd` - 向量加法 (Vec, 动态延迟)
- `hivm.hir.vmul` - 向量乘法 (Vec, 动态延迟)
- `hivm.hir.matmul` - 矩阵乘法 (Cube, 动态延迟)
- `hivm.hir.store` - UB 到 GM 数据存储 (MTE, 动态延迟)

## 7. 文件结构

```
ascendir_parser/
├── include/
│   ├── Isa.h                       # ISA 基类（含硬件特性描述）
│   ├── IsaRegistry.h               # ISA 注册表（简化版）
│   ├── IsaSequence.h               # ISA 序列
│   ├── IsaExecutor.h               # ISA 执行器（PC/Cycle 管理）
│   ├── ExecutionContext.h          # 执行上下文（值存储）
│   ├── ExecutionUnit.h             # 执行单元定义
│   └── Instructions/
│       ├── ArithIsas.h             # arith 指令类
│       ├── ControlFlowIsas.h       # 控制流指令类
│       ├── FuncIsas.h              # func 指令类
│       ├── HivmIsas.h              # hivm 指令类
│       └── YieldIsa.h              # yield 赋值指令
│
├── src/
│   ├── Isa.cpp                     # ISA 基类实现
│   ├── IsaRegistry.cpp             # 注册表实现
│   ├── IsaSequence.cpp             # 序列构建和管理
│   ├── IsaExecutor.cpp             # PC 驱动执行引擎
│   ├── ExecutionContext.cpp        # 运行时状态管理
│   └── main.cpp                    # 主程序
│
└── test/
    ├── test_simple.mlir            # 简单测试
    ├── test_scf_for.mlir           # 循环测试
    ├── test_scf_if.mlir            # 条件分支测试
    ├── test_hivm_basic.mlir        # HIVM 方言测试
    ├── test_multi_component.mlir   # 多组件协作测试
    ├── test_matmul_pipeline.mlir   # 矩阵乘法流水线测试
    └── test_pipeline.mlir          # 完整流水线测试
```

## 8. 扩展机制

### 8.1 添加新指令

```cpp
// 1. 定义 Isa 子类
class MyCustomIsa : public Isa {
public:
    // 执行单元
    ExecutionUnitType getExecutionUnit() const override {
        return ExecutionUnitType::Vec;
    }
    
    // 基础延迟
    uint64_t getLatency() const override {
        return 5;
    }
    
    // 动态延迟（可选）
    HardwareCharacteristics getHardwareCharacteristics(ExecutionContext& ctx) override {
        HardwareCharacteristics hw;
        hw.targetUnit = ExecutionUnitType::Vec;
        hw.latency = 5;  // 或根据数据大小计算
        return hw;
    }
    
    // 执行逻辑
    IsaExecuteResult execute(ExecutionContext& ctx) override {
        // 实现指令逻辑
        return IsaExecuteResult::continueExecution();
    }
    
    std::string getDescription() const override {
        return "mydialect.myop";
    }
};

// 2. 注册指令
inline void registerMyDialect() {
    REGISTER_ISA("mydialect.myop", MyCustomIsa);
}
```

## 9. 输出格式

### 9.1 详细输出示例

```
Starting simulation (mode: asynchronous)...

Cycle[1] PC[0] IsaID[0] memref.alloc
  [memref.alloc] Allocated 256 bytes for memref<128xf16>

Cycle[2] PC[1] IsaID[1] hivm.hir.load
  [hivm.hir.load] GM -> UB on MTE
    Data size: 256 bytes (128 elements)
  Dispatched task 0 on MTE unit, latency=10

Cycle[3] PC[2] IsaID[2] memref.alloc
  [memref.alloc] Allocated 256 bytes for memref<128xf16>

...

Simulation completed.
Total cycles: 56
```

## 10. 架构演进历史

### 10.1 最新变更（当前版本）

**职责分离重构：**

| 组件 | 之前 | 之后 |
|------|------|------|
| ExecutionContext | PC、Cycle、执行单元、值存储 | 仅值存储和调用栈 |
| IsaExecutor | 执行逻辑 | PC、Cycle、执行单元、阻塞管理 |
| IsaRegistry | 存储延迟、单元、计算器 | 仅创建 Isa 实例 |
| Isa 类 | 仅执行逻辑 | 执行逻辑 + 硬件特性描述 |

**关键改进：**
1. `Isa` 类自描述硬件特性（`getExecutionUnit()`、`getLatency()`、`getHardwareCharacteristics()`）
2. `ExecutionContext` 简化为纯数据存储
3. `IsaExecutor` 集中管理 PC、Cycle 和执行单元
4. `IsaRegistry` 简化为工厂模式

## 11. 总结

本设计提供了一个完整的 MLIR 仿真器框架，具有以下特点：

1. **PC 驱动**：简单清晰的执行模型
2. **线性指令序列**：避免嵌套执行，支持指令级调度
3. **Isa 自描述**：每个指令类定义自己的硬件特性
4. **模块化设计**：各组件职责明确，易于维护
5. **指令耗时模拟**：支持 cycle 级别的性能分析
6. **控制流展平**：支持 scf.for 和 scf.if 展平
7. **多组件并行执行**：支持 Scalar、MTE、Cube、Vec 组件并行工作
8. **数据大小相关延迟**：根据数据量动态计算延迟
