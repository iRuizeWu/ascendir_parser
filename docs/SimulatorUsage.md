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

| 操作 | 说明 | 耗时 (cycles) |
|------|------|---------------|
| `arith.constant` | 常量定义 | 1 |
| `arith.addi` | 整数加法 | 1 |
| `arith.subi` | 整数减法 | 1 |
| `arith.muli` | 整数乘法 | 3 |
| `arith.divsi` | 整数除法 | 10 |
| `arith.addf` | 浮点加法 | 2 |
| `arith.subf` | 浮点减法 | 2 |
| `arith.mulf` | 浮点乘法 | 4 |
| `arith.divf` | 浮点除法 | 12 |
| `arith.cmpi` | 整数比较 | 1 |
| `arith.index_cast` | 类型转换 | 1 |

### func dialect

| 操作 | 说明 | 耗时 (cycles) |
|------|------|---------------|
| `func.call` | 函数调用 | 5 |
| `func.return` | 函数返回 | 1 |

### scf dialect

| 操作 | 说明 |
|------|------|
| `scf.for` | for循环（展平为ForInit/ForCondition/ForIncrement） |
| `scf.if` | 条件分支（展平为IfCondition） |
| `scf.yield` | 产生值 |

### memref dialect

| 操作 | 说明 | 耗时 (cycles) |
|------|------|---------------|
| `memref.alloc` | 内存分配 | 1 |

### hivm dialect

| 操作 | 说明 | 组件 | 延迟模型 |
|------|------|------|----------|
| `hivm.hir.load` | GM到UB数据加载 | MTE | base=10, 128 bytes/cycle |
| `hivm.hir.vadd` | 向量加法 | Vec | base=5, 256 bytes/cycle |
| `hivm.hir.vmul` | 向量乘法 | Vec | base=5, 256 bytes/cycle |
| `hivm.hir.matmul` | 矩阵乘法 | Cube | base=20, 512 bytes/cycle |
| `hivm.hir.store` | UB到GM数据存储 | MTE | base=10, 128 bytes/cycle |

**延迟计算公式**: `latency = max(baseLatency, dataSize / bytesPerCycle)`

**示例**:
- `memref<128xf16>` 的 load: dataSize=256 bytes, latency=max(10, 256/128)=10 cycles
- `memref<64x64xf16>` 的 matmul: dataSize=16384 bytes, latency=max(20, 16384/512)=32 cycles

## 输出格式

### 异步模式输出

```
Starting simulation (mode: asynchronous)...

[cycle=0] PC 0: Normal: %ubA = memref.alloc() : memref<128xf16>
  [memref.alloc] Allocated 256 bytes for memref<128xf16>
[cycle=1] Scalar advanced 1 cycles

[cycle=1] PC 1: Normal: "hivm.hir.load"(%arg0, %ubA) : ...
  [hivm.hir.load] GM -> UB on MTE
    Data size: 256 bytes (128 elements)
    Initialized 128 elements (test data): [0, 1, 2, 3, ..., 126, 127]
  [Dispatch] Task to MTE, duration=10 cycles, dataSize=256 bytes
  Active tasks:
    - hivm.hir.load on MTE (complete at cycle 11)

[cycle=2] PC 2: Normal: %ubB = memref.alloc() : memref<128xf16>
  [memref.alloc] Allocated 256 bytes for memref<128xf16>

[cycle=3] PC 3: Normal: "hivm.hir.load"(%arg1, %ubB) : ...
  [hivm.hir.load] GM -> UB on MTE
    Data size: 256 bytes (128 elements)
  [Dispatch] Task to MTE, duration=10 cycles, dataSize=256 bytes
  Active tasks:
    - hivm.hir.load on MTE (complete at cycle 11)
    - hivm.hir.load on MTE (complete at cycle 13)

...

Simulation completed.
Final PC: 8
Total cycles: 56
All units idle: yes
```

### 同步模式输出

```
Starting simulation (mode: synchronous)...

[cycle=0] PC 0: Normal: %ubA = memref.alloc() : memref<128xf16>
  [memref.alloc] Allocated 256 bytes for memref<128xf16>
[cycle=1] Scalar advanced 1 cycles

[cycle=1] PC 1: Normal: "hivm.hir.load"(%arg0, %ubA) : ...
  [hivm.hir.load] GM -> UB on MTE
    Data size: 256 bytes (128 elements)
[cycle=11] Scalar advanced 10 cycles

...

Simulation completed.
Final PC: 8
Total cycles: 326
All units idle: yes
```

### 输出说明

- `[cycle=X] PC Y: ...` - 在cycle X时开始执行PC Y处的指令
- `[Dispatch] Task to <unit>` - 任务分发到对应组件
- `Active tasks:` - 当前活跃的后台任务
- `complete at cycle X` - 任务完成时间
- `Total cycles: N` - 总耗时统计
- `All units idle: yes/no` - 所有组件是否空闲

## 测试示例

### test_simple.mlir
简单的常量定义测试

### test_compute.mlir
整数运算测试（加减乘）

### test_float.mlir
浮点运算测试

### test_compare.mlir
比较运算测试

### test_for.mlir
循环测试

### test_nested_for.mlir
嵌套循环测试

### test_if.mlir
条件分支测试

### test_call.mlir
函数调用测试

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

1. **InstructionSequence** - 线性指令序列，支持PC管理
2. **ExecutionContext** - 运行时状态管理（值存储、PC、调用栈、cycle计数、多组件管理）
3. **InstructionExecutor** - PC驱动的执行引擎
4. **InstructionRegistry** - 指令注册表，支持动态扩展和耗时配置
5. **ExecutionUnit** - 执行单元（Scalar、MTE、Cube、Vec）

### 多组件执行模型

```
┌─────────────────────────────────────────┐
│         Scalar (主调度单元)              │
│  - PC驱动执行                           │
│  - 标量运算：直接执行                   │
│  - 其他指令：分发到对应组件             │
└─────────────────────────────────────────┘
         ↓ dispatch    ↓ dispatch    ↓ dispatch
┌─────────────┐ ┌─────────────┐ ┌─────────────┐
│     MTE     │ │    Cube     │ │     Vec     │
│ 数据搬运    │ │  矩阵乘法   │ │  向量运算   │
│ load/store  │ │  matmul     │ │ vadd/vmul   │
└─────────────┘ └─────────────┘ └─────────────┘
```

## 扩展新指令

在 `src/Instructions/` 目录下添加新的指令处理器：

```cpp
// 1. 固定延迟指令（Scalar组件）
void executeMyOp(mlir::Operation* op, ExecutionContext& ctx) {
    // 实现指令逻辑
}
REGISTER_INSTRUCTION_WITH_LATENCY("mydialect.myop", executeMyOp, 10);

// 2. 指定组件的指令
void executeVecOp(mlir::Operation* op, ExecutionContext& ctx) {
    // 实现向量运算
}
REGISTER_INSTRUCTION_ON_UNIT("mydialect.vecop", executeVecOp, 5, ExecutionUnitType::Vec);

// 3. 数据大小相关延迟的指令
size_t calculateDataSize(mlir::Operation* op, ExecutionContext& ctx) {
    // 计算数据大小
    return dataSize;
}

ComponentLatencyModel model;
model.baseLatency = 10;
model.bytesPerCycle = 256.0;
model.dataSizeDependent = true;

REGISTER_INSTRUCTION_WITH_DATA_SIZE("mydialect.mteop", executeMteOp,
                                    calculateDataSize, model,
                                    ExecutionUnitType::MTE);
```

## 下一步计划

- [ ] 添加更多HIVM操作支持
- [ ] 实现流水线模拟
- [ ] 添加调试支持（断点、单步执行）
- [ ] 性能分析和优化
