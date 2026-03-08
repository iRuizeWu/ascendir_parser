# MLIR仿真器项目

## 项目概述

本项目实现了一个MLIR仿真器，能够逐条执行MLIR指令，支持控制流展平、函数调用、HIVM方言和指令耗时模拟。

## 快速开始

### 编译

```bash
cd /home/ruize/code/github/ascendir_parser
rm -rf build/*
cmake -G Ninja -S . -B build \
    -DLLVM_DIR=/home/ruize/code/github/AscendNPU-IR/build \
    -DMLIR_DIR=/home/ruize/code/github/AscendNPU-IR/build/lib/cmake/mlir
cmake --build build -j4
```

### 运行测试

```bash
./test/run_tests.sh
```

### 使用示例

```bash
# 执行MLIR文件
./build/src/ascendir_parser test/test_compute.mlir --simulate

# 显示指令序列
./build/src/ascendir_parser test/test_compute.mlir --simulate --dump-sequence

# 详细执行信息（含cycle耗时）
./build/src/ascendir_parser test/test_hivm_basic.mlir --simulate --verbose
```

## 已实现功能

### ✅ 阶段1：基础框架
- 统一指令表示（Instruction）
- 指令序列管理（InstructionSequence）
- 执行上下文（ExecutionContext）
- 指令注册表（InstructionRegistry）
- PC驱动执行引擎（InstructionExecutor）

### ✅ 阶段2：算术运算
- 整数运算：addi, subi, muli, divsi
- 浮点运算：addf, subf, mulf, divf
- 比较运算：cmpi（支持所有10种谓词）
- 常量定义：arith.constant
- 类型转换：arith.index_cast

### ✅ 阶段3：控制流展平
- scf.for 循环展平
- scf.if 条件分支展平
- 嵌套控制流支持
- 循环迭代变量管理
- iter_args 支持

### ✅ 阶段4：函数调用
- func.call 函数调用
- func.return 函数返回
- 调用栈管理
- 参数传递

### ✅ 阶段5：HIVM方言
- memref.alloc 内存分配
- hivm.hir.load GM到UB数据加载
- hivm.hir.vadd 向量加法
- hivm.hir.store UB到GM数据存储
- f16精度浮点支持

### ✅ 阶段6：指令耗时模拟
- 每条指令独立的耗时配置
- cycle级别的时间模拟
- 总耗时统计

## 指令耗时配置

| 指令 | 耗时 (cycles) |
|------|---------------|
| memref.alloc | 1 |
| hivm.hir.load | 100 |
| hivm.hir.vadd | 50 |
| hivm.hir.store | 100 |
| arith.addi/subi | 1 |
| arith.muli | 3 |
| arith.divsi | 10 |
| arith.addf/subf | 2 |
| arith.mulf | 4 |
| arith.divf | 12 |
| func.call | 5 |
| func.return | 1 |

## 输出示例

```
Starting simulation...

[cycle=0] PC 0: Normal: %alloc = memref.alloc() : memref<16xf16>
  [memref.alloc] Allocated 32 bytes for memref<16xf16>
[cycle=1] retired

[cycle=1] PC 1: Normal: "hivm.hir.load"(%arg0, %alloc) : ...
  [hivm.hir.load] GM -> UB
    Initialized 16 elements (test data): [0, 1, 2, 3, ..., 14, 15]
[cycle=101] retired

[cycle=101] PC 2: Normal: %alloc_0 = memref.alloc() : memref<16xf16>
  [memref.alloc] Allocated 32 bytes for memref<16xf16>
[cycle=102] retired

...

Simulation completed.
Final PC: 8
Total cycles: 354
```

## 项目结构

```
ascendir_parser/
├── include/
│   ├── Instruction.h              # 指令表示
│   ├── InstructionSequence.h      # 指令序列
│   ├── InstructionRegistry.h      # 指令注册表（含耗时）
│   ├── ExecutionContext.h         # 执行上下文（含cycle）
│   └── InstructionExecutor.h      # 指令执行器
│
├── src/
│   ├── InstructionSequence.cpp
│   ├── ExecutionContext.cpp
│   ├── InstructionRegistry.cpp
│   ├── InstructionExecutor.cpp
│   ├── Instructions/
│   │   ├── ArithInstructions.cpp  # arith操作
│   │   ├── FuncInstructions.cpp   # func操作
│   │   └── HivmInstructions.cpp   # hivm操作
│   └── main.cpp                   # 主程序
│
├── test/
│   ├── test_simple.mlir           # 简单测试
│   ├── test_arith.mlir            # 算术测试
│   ├── test_for.mlir              # 循环测试
│   ├── test_nested_for.mlir       # 嵌套循环测试
│   ├── test_if.mlir               # 条件分支测试
│   ├── test_call.mlir             # 函数调用测试
│   └── test_hivm_basic.mlir       # HIVM方言测试
│
└── docs/
    ├── SimulatorDesign.md         # 设计文档
    ├── SimulatorUsage.md          # 使用说明
    └── SimulatorSummary.md        # 开发总结
```

## 架构特点

### 1. PC驱动执行模型
- 简单清晰的执行流程
- 支持线性指令序列
- 支持跳转和控制流

### 2. 统一指令管理
- 注册表模式支持动态扩展
- 易于添加新dialect
- 模块化的指令处理器
- 指令耗时配置

### 3. 类型安全
- 使用APInt/APFloat支持MLIR类型系统
- 正确处理SSA值
- 类型安全的值存储

### 4. 控制流展平
- scf.for → ForInit + ForCondition + ForIncrement
- scf.if → IfCondition + Jump
- 支持嵌套控制流

## 扩展新指令

在 `src/Instructions/` 目录下添加新的指令处理器：

```cpp
void executeMyOp(mlir::Operation* op, ExecutionContext& ctx) {
    // 实现指令逻辑
}

// 注册指令（含耗时）
REGISTER_INSTRUCTION_WITH_LATENCY("mydialect.myop", executeMyOp, 10);
```

## 文档

- [SimulatorDesign.md](docs/SimulatorDesign.md) - 详细设计文档
- [SimulatorUsage.md](docs/SimulatorUsage.md) - 使用说明
- [SimulatorSummary.md](docs/SimulatorSummary.md) - 开发总结

## 技术栈

- C++17
- LLVM/MLIR 19+
- CMake 3.28+
- Ninja

## 许可证

本项目遵循相关开源协议。
