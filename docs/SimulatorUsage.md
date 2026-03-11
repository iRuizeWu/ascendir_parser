# MLIR仿真器使用说明

## 概述

MLIR仿真器是一个能够逐条执行MLIR指令的工具，支持arith、func、scf、memref和hivm dialect，并提供指令耗时模拟和多组件并行执行功能。

## 编译

```bash
cd /home/ruize/code/github/ascendir_parser
rm -rf build/*
cmake -G Ninja -S . -B build \
    -DLLVM_DIR=/home/ruize/code/github/AscendNPU-IR/build \
    -DMLIR_DIR=/home/ruize/code/github/AscendNPU-IR/build/lib/cmake/mlir
cmake --build build -j4
```

## 使用方法

### 基本仿真

```bash
./build/src/ascendir_parser test.mlir --simulate
```

### 显示指令序列

```bash
./build/src/ascendir_parser test.mlir --simulate --dump-sequence
```

### 详细执行信息（含cycle耗时）

```bash
./build/src/ascendir_parser test.mlir --simulate --verbose
```

### 异步模式（默认）

```bash
# 异步模式：Scalar分发任务后继续执行，其他组件并行工作
./build/src/ascendir_parser test.mlir --simulate --verbose
```

### 同步模式

```bash
# 同步模式：每条指令等待完成后才继续
./build/src/ascendir_parser test.mlir --simulate --verbose --sync
```

### 组合使用

```bash
./build/src/ascendir_parser test.mlir --simulate --dump-sequence --verbose
```

## 支持的操作

### arith dialect

| 操作 | 说明 | 耗时 (cycles) | 执行单元 |
|------|------|---------------|----------|
| `arith.constant` | 常量定义 | 1 | Scalar |
| `arith.addi` | 整数加法 | 1 | Scalar |
| `arith.subi` | 整数减法 | 1 | Scalar |
| `arith.muli` | 整数乘法 | 3 | Scalar |
| `arith.divsi` | 整数除法 | 10 | Scalar |
| `arith.addf` | 浮点加法 | 2 | Scalar |
| `arith.subf` | 浮点减法 | 2 | Scalar |
| `arith.mulf` | 浮点乘法 | 4 | Scalar |
| `arith.divf` | 浮点除法 | 12 | Scalar |
| `arith.cmpi` | 整数比较 | 1 | Scalar |
| `arith.index_cast` | 类型转换 | 5 | Scalar |

### func dialect

| 操作 | 说明 | 耗时 (cycles) | 执行单元 |
|------|------|---------------|----------|
| `func.call` | 函数调用 | 5 | Scalar |
| `func.return` | 函数返回 | 1 | Scalar |

### scf dialect

| 操作 | 说明 |
|------|------|
| `scf.for` | for循环（展平为ForInit/ForCondition/ForIncrement） |
| `scf.if` | 条件分支（展平为IfCondition） |
| `scf.yield` | 产生值 |

### memref dialect

| 操作 | 说明 | 耗时 (cycles) | 执行单元 |
|------|------|---------------|----------|
| `memref.alloc` | 内存分配 | 1 | Scalar |

### hivm dialect

| 操作 | 说明 | 执行单元 | 延迟模型 |
|------|------|----------|----------|
| `hivm.hir.load` | GM到UB数据加载 | MTE | base=10, 动态计算 |
| `hivm.hir.vadd` | 向量加法 | Vec | base=5, 动态计算 |
| `hivm.hir.vmul` | 向量乘法 | Vec | base=5, 动态计算 |
| `hivm.hir.matmul` | 矩阵乘法 | Cube | base=20, 动态计算 |
| `hivm.hir.store` | UB到GM数据存储 | MTE | base=10, 动态计算 |

**延迟计算**：每个 Isa 类通过 `getHardwareCharacteristics()` 方法动态计算延迟，可根据数据大小调整。

## 输出格式

### 详细输出示例

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

### 输出说明

- `Cycle[X] PC[Y] IsaID[Z] ...` - 在 cycle X 时开始执行 PC Y 处的指令
- `Dispatched task N on <unit> unit` - 任务分发到对应组件
- `Total cycles: N` - 总耗时统计

## 测试示例

### test_simple.mlir
简单的常量定义测试

### test_compute.mlir
整数运算测试（加减乘）

### test_float.mlir
浮点运算测试

### test_compare.mlir
比较运算测试

### test_scf_for.mlir
循环测试

### test_nested_for.mlir
嵌套循环测试

### test_scf_if.mlir
条件分支测试

### test_hivm_basic.mlir
HIVM方言测试（向量加法）

### test_multi_component.mlir
多组件协作测试（MTE + Vec）

### test_matmul_pipeline.mlir
矩阵乘法流水线测试（MTE + Cube + Vec）

### test_pipeline.mlir
完整流水线测试（load + vmul + vadd + store）

## 架构

仿真器采用PC驱动的执行模型，支持多组件并行执行：

### 核心组件

| 组件 | 职责 |
|------|------|
| ExecutionContext | 值存储、调用栈管理 |
| IsaExecutor | PC/Cycle 管理、执行单元实例化、阻塞管理 |
| IsaRegistry | 创建 Isa 实例（工厂模式） |
| Isa 类 | 执行逻辑 + 硬件特性描述 |

### 多组件执行模型

```
┌─────────────────────────────────────────┐
│         IsaExecutor (主调度器)           │
│  - PC 管理                              │
│  - Cycle 管理                           │
│  - 任务分发                             │
└─────────────────────────────────────────┘
         ↓ dispatch    ↓ dispatch    ↓ dispatch
┌─────────────┐ ┌─────────────┐ ┌─────────────┐
│     MTE     │ │    Cube     │ │     Vec     │
│ 数据搬运    │ │  矩阵乘法   │ │  向量运算   │
│ load/store  │ │  matmul     │ │ vadd/vmul   │
└─────────────┘ └─────────────┘ └─────────────┘
```

### Isa 类自描述硬件特性

每个 Isa 子类定义自己的硬件特性：

```cpp
class HivmHirLoadIsa : public Isa {
public:
    ExecutionUnitType getExecutionUnit() const override {
        return ExecutionUnitType::MTE;
    }
    
    uint64_t getLatency() const override {
        return 10;
    }
    
    HardwareCharacteristics getHardwareCharacteristics(ExecutionContext& ctx) override {
        // 动态计算延迟
    }
};
```

## 扩展新指令

在 `include/Instructions/` 目录下添加新的ISA指令类：

```cpp
// 1. 定义新的ISA类
class MyCustomIsa : public Isa {
public:
    ExecutionUnitType getExecutionUnit() const override {
        return ExecutionUnitType::Vec;
    }
    
    uint64_t getLatency() const override {
        return 5;
    }
    
    HardwareCharacteristics getHardwareCharacteristics(ExecutionContext& ctx) override {
        HardwareCharacteristics hw;
        hw.targetUnit = ExecutionUnitType::Vec;
        hw.latency = 5;  // 或根据数据大小计算
        return hw;
    }
    
    IsaExecuteResult execute(ExecutionContext& ctx) override {
        // 实现指令逻辑
        return IsaExecuteResult::continueExecution();
    }
    
    std::string getDescription() const override {
        return "mydialect.myop";
    }
};

// 2. 注册指令
REGISTER_ISA("mydialect.myop", MyCustomIsa);
```

## 下一步计划

- [ ] 添加更多HIVM操作支持
- [ ] 实现流水线模拟
- [ ] 添加调试支持（断点、单步执行）
- [ ] 性能分析和优化
