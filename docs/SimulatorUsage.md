# MLIR仿真器使用说明

## 概述

MLIR仿真器是一个能够逐条执行MLIR指令的工具，支持arith、func、scf、memref和hivm dialect，并提供指令耗时模拟功能。

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

| 操作 | 说明 | 耗时 (cycles) |
|------|------|---------------|
| `hivm.hir.load` | GM到UB数据加载 | 100 |
| `hivm.hir.vadd` | 向量加法 | 50 |
| `hivm.hir.store` | UB到GM数据存储 | 100 |

## 输出格式

### 详细模式输出

```
Starting simulation...

[cycle=0] PC 0: Normal: %alloc = memref.alloc() : memref<16xf16>
  [memref.alloc] Allocated 32 bytes for memref<16xf16>
[cycle=1] retired

[cycle=1] PC 1: Normal: "hivm.hir.load"(%arg0, %alloc) : (memref<16xf16>, memref<16xf16>) -> ()
  [hivm.hir.load] GM -> UB
    Initialized 16 elements (test data): [0, 1, 2, 3, ..., 14, 15]
[cycle=101] retired

[cycle=101] PC 2: Normal: %alloc_0 = memref.alloc() : memref<16xf16>
  [memref.alloc] Allocated 32 bytes for memref<16xf16>
[cycle=102] retired

[cycle=102] PC 3: Normal: "hivm.hir.load"(%arg1, %alloc_0) : ...
  [hivm.hir.load] GM -> UB
    Initialized 16 elements (test data): [0, 1, 2, 3, ..., 14, 15]
[cycle=202] retired

[cycle=202] PC 4: Normal: %alloc_1 = memref.alloc() : memref<16xf16>
  [memref.alloc] Allocated 32 bytes for memref<16xf16>
[cycle=203] retired

[cycle=203] PC 5: Normal: "hivm.hir.vadd"(%alloc, %alloc_0, %alloc_1) : ...
  [hivm.hir.vadd] Vector addition
    Result [16 elements]: [0, 2, 4, 6, ..., 28, 30]
[cycle=253] retired

[cycle=253] PC 6: Normal: "hivm.hir.store"(%alloc_1, %arg2) : ...
  [hivm.hir.store] UB -> GM
    Stored 32 bytes [16 elements]: [0, 2, 4, 6, ..., 28, 30]
[cycle=353] retired

[cycle=353] PC 7: Normal: func.return
[cycle=354] retired

Simulation completed.
Final PC: 8
Total cycles: 354
```

### 输出说明

- `[cycle=X] PC Y: ...` - 在cycle X时开始执行PC Y处的指令
- `[cycle=X] retired` - 在cycle X时指令执行完成
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

## 架构

仿真器采用PC驱动的执行模型：

1. **InstructionSequence** - 线性指令序列，支持PC管理
2. **ExecutionContext** - 运行时状态管理（值存储、PC、调用栈、cycle计数）
3. **InstructionExecutor** - PC驱动的执行引擎
4. **InstructionRegistry** - 指令注册表，支持动态扩展和耗时配置

## 扩展新指令

在 `src/Instructions/` 目录下添加新的指令处理器：

```cpp
void executeMyOp(mlir::Operation* op, ExecutionContext& ctx) {
    // 实现指令逻辑
}

// 注册指令（含耗时）
REGISTER_INSTRUCTION_WITH_LATENCY("mydialect.myop", executeMyOp, 10);
```

## 下一步计划

- [ ] 添加更多HIVM操作支持
- [ ] 实现流水线模拟
- [ ] 添加调试支持（断点、单步执行）
- [ ] 性能分析和优化
