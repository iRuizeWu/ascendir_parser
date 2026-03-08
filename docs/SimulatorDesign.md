# MLIR仿真器设计文档

## 1. 概述

### 1.1 目标
构建一个MLIR仿真器，能够逐条执行MLIR指令，支持控制流展平、函数调用、HIVM方言和指令耗时模拟。

### 1.2 核心特性
- 基于PC（Program Counter）驱动的执行模型
- 将嵌套控制流展平为线性指令序列
- 统一的指令管理机制
- 支持多种MLIR dialect
- 指令耗时模拟（cycle级别）

## 2. 系统架构

### 2.1 整体架构

```
┌─────────────────────────────────────────────────────────┐
│                    MLIR Simulator                        │
├─────────────────────────────────────────────────────────┤
│                                                           │
│  ┌──────────────┐   ┌──────────────────────────────┐   │
│  │    Parser    │ → │  SequenceBuilder             │   │
│  │   (现有模块)  │   │  (控制流展平 & 指令序列构建)  │   │
│  └──────────────┘   └──────────────────────────────┘   │
│         ↓                        ↓                       │
│  ┌──────────────┐   ┌──────────────────────────────┐   │
│  │ MLIR Module  │   │  InstructionSequence         │   │
│  │              │   │  (线性指令序列 + PC管理)      │   │
│  └──────────────┘   └──────────────────────────────┘   │
│                                ↓                         │
│                    ┌──────────────────────────────┐     │
│                    │  InstructionExecutor         │     │
│                    │  (PC驱动执行引擎)             │     │
│                    └──────────────────────────────┘     │
│                                ↓                         │
│                    ┌──────────────────────────────┐     │
│                    │  ExecutionContext            │     │
│                    │  (运行时状态 + Cycle计数)     │     │
│                    └──────────────────────────────┘     │
│                                                           │
└─────────────────────────────────────────────────────────┘
```

### 2.2 执行流程

```
1. 解析MLIR文件 → ModuleOp
2. 构建指令序列（控制流展平 + PC分配）
3. 初始化执行上下文（PC=0, cycle=0）
4. 循环：fetch instruction at PC → execute → update PC & cycle
5. 结束（halt或执行完毕）
```

## 3. 核心数据结构

### 3.1 统一指令表示

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

### 3.2 指令信息（含耗时）

```cpp
struct InstructionInfo {
    using ExecuteFunc = std::function<void(mlir::Operation*, ExecutionContext&)>;
    ExecuteFunc handler;
    uint64_t latency;                   // 指令耗时（cycles）
    
    InstructionInfo() : latency(1) {}
    InstructionInfo(ExecuteFunc h, uint64_t lat = 1) : handler(h), latency(lat) {}
};
```

### 3.3 指令注册表

```cpp
class InstructionRegistry {
public:
    void registerHandler(const std::string& opName, ExecuteFunc handler, uint64_t latency = 1);
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
```

### 3.4 执行上下文（含Cycle计数）

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
    
    // 执行控制
    void halt() { halted = true; }
    bool isHalted() const { return halted; }
    
    // 调用栈管理
    void pushCallFrame(uint64_t returnPC);
    uint64_t popCallFrame();
};
```

### 3.5 指令执行器

```cpp
class InstructionExecutor {
    InstructionSequence sequence;
    ExecutionContext ctx;
    
public:
    void load(mlir::ModuleOp module);
    bool executeNext();
    void run();
    
    uint64_t getCurrentPC() const { return ctx.getPC(); }
    bool isHalted() const { return ctx.isHalted(); }
    ExecutionContext& getContext() { return ctx; }
    
private:
    void executeInstruction(Instruction* inst);
    uint64_t executeOp(mlir::Operation* op);
    uint64_t executeNormalInstruction(Instruction* inst);
    void executeJumpInstruction(Instruction* inst);
    void executeConditionalJumpInstruction(Instruction* inst);
    void executeForInitInstruction(Instruction* inst);
    void executeForConditionInstruction(Instruction* inst);
    void executeForIncrementInstruction(Instruction* inst);
    void executeReturnInstruction();
};
```

## 4. 指令耗时模拟

### 4.1 耗时配置

每条指令都有独立的耗时配置：

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

### 4.2 执行流程

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
        return executeOp(inst->mlirOp);
    }
    advancePC();
    return 1;
}

uint64_t InstructionExecutor::executeOp(mlir::Operation* op) {
    std::string opName = op->getName().getStringRef().str();
    
    auto& registry = getGlobalRegistry();
    if (registry.hasHandler(opName)) {
        auto* info = registry.getInstructionInfo(opName);
        if (info && info->handler) {
            info->handler(op, ctx);
            return info->latency;
        }
    }
    return 1;
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
- `hivm.hir.load` - GM到UB数据加载
- `hivm.hir.vadd` - 向量加法
- `hivm.hir.store` - UB到GM数据存储

## 7. 文件结构

```
ascendir_parser/
├── include/
│   ├── Instruction.h               # 指令表示
│   ├── InstructionSequence.h       # 指令序列
│   ├── InstructionRegistry.h       # 指令注册表（含耗时）
│   ├── ExecutionContext.h          # 执行上下文（含cycle）
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
│   │   └── HivmInstructions.cpp    # hivm操作
│   └── main.cpp                    # 主程序
│
└── test/
    ├── test_simple.mlir            # 简单测试
    ├── test_for.mlir               # 循环测试
    ├── test_if.mlir                # 条件分支测试
    ├── test_call.mlir              # 函数调用测试
    └── test_hivm_basic.mlir        # HIVM方言测试
```

## 8. 扩展机制

### 8.1 添加新指令

```cpp
// 1. 实现处理器函数
void executeMyCustomOp(mlir::Operation* op, ExecutionContext& ctx) {
    auto myOp = llvm::cast<MyDialect::MyOp>(op);
    // 执行逻辑
}

// 2. 注册指令（含耗时）
REGISTER_INSTRUCTION_WITH_LATENCY("mydialect.myop", executeMyCustomOp, 10);
```

### 8.2 添加新Dialect

```cpp
void registerMyDialect() {
    REGISTER_INSTRUCTION_WITH_LATENCY("mydialect.op1", executeOp1, 5);
    REGISTER_INSTRUCTION_WITH_LATENCY("mydialect.op2", executeOp2, 10);
}
```

## 9. 输出格式

### 9.1 详细模式

```
[cycle=0] PC 0: Normal: %alloc = memref.alloc() : memref<16xf16>
  [memref.alloc] Allocated 32 bytes for memref<16xf16>
[cycle=1] retired

[cycle=1] PC 1: Normal: "hivm.hir.load"(%arg0, %alloc) : ...
  [hivm.hir.load] GM -> UB
    Initialized 16 elements (test data): [0, 1, 2, 3, ..., 14, 15]
[cycle=101] retired
```

### 9.2 格式说明

- `[cycle=X]` - 当前cycle数
- `PC Y` - 当前程序计数器
- `Normal/Jump/ForCondition...` - 指令类型
- `retired` - 指令执行完成

## 10. 性能优化方向

### 10.1 指令缓存
- 缓存频繁执行的指令序列
- 预取接下来的N条指令

### 10.2 JIT编译
- 将热点指令序列编译为机器码
- 使用LLVM JIT

### 10.3 并行执行
- 识别无依赖的指令
- 多线程执行

### 10.4 流水线模拟
- 指令级并行
- 乱序执行模拟

## 11. 总结

本设计提供了一个完整的MLIR仿真器框架，具有以下特点：

1. **PC驱动**：简单清晰的执行模型
2. **线性指令序列**：避免嵌套执行，支持指令级调度
3. **统一指令管理**：注册表模式，易于扩展
4. **模块化设计**：各组件职责明确，易于维护
5. **指令耗时模拟**：支持cycle级别的性能分析
6. **控制流展平**：支持scf.for和scf.if展平

该设计为后续的指令发射优化提供了良好的基础框架。
