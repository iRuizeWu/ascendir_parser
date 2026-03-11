# MLIR仿真器项目

## 项目概述

本项目实现了一个MLIR仿真器，能够逐条执行MLIR指令，支持控制流展平、函数调用、HIVM方言、指令耗时模拟和多组件并行执行。

## 快速开始

### 编译

```bash
cd /home/ruize/code/github/ascendir_parser
rm -rf build/*
cmake -G Ninja -S . -B build \
    -DLLVM_DIR=/home/ruize/code/github/AscendNPU-IR/build \
    -DMLIR_DIR=/home/ruize/code/github/AscendNPU-IR/build/lib/cmake/mlir \
    -DBISHENGIR_DIR=/home/ruize/code/github/AscendNPU-IR/build \
    -DBISHENGIR_SOURCE_DIR=/home/ruize/code/github/AscendNPU-IR
cmake --build build -j4
```

### 运行测试

```bash
./test/run_tests.sh
```

### 使用示例

```bash
# 执行MLIR文件（异步模式）
./build/src/ascendir_parser test/test_compute.mlir --simulate

# 显示指令序列
./build/src/ascendir_parser test/test_compute.mlir --simulate --dump-sequence

# 详细执行信息（含cycle耗时）
./build/src/ascendir_parser test/test_hivm_basic.mlir --simulate --verbose

# 同步模式（等待每条指令完成）
./build/src/ascendir_parser test/test_pipeline.mlir --simulate --verbose --sync
```

## 已实现功能

### ✅ 阶段1：基础框架
- Isa 基类（统一指令表示）
- 指令序列管理（IsaSequence）
- 执行上下文（ExecutionContext）
- 指令注册表（IsaRegistry）
- PC驱动执行引擎（IsaExecutor）

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

### ✅ 阶段7：多组件并行执行
- 执行单元类型：Scalar、MTE、Cube、Vec
- 异步任务分发机制
- 数据大小相关延迟计算
- 组件延迟模型配置
- 同步/异步执行模式切换

## 指令耗时配置

### 固定延迟指令（Scalar组件）

| 指令 | 耗时 (cycles) |
|------|---------------|
| memref.alloc | 1 |
| arith.addi/subi | 1 |
| arith.muli | 3 |
| arith.divsi | 10 |
| arith.addf/subf | 2 |
| arith.mulf | 4 |
| arith.divf | 12 |
| func.call | 5 |
| func.return | 1 |

### 数据大小相关延迟指令

| 指令 | 组件 | 基础延迟 | 吞吐量 |
|------|------|----------|--------|
| hivm.hir.load | MTE | 10 cycles | 128 bytes/cycle |
| hivm.hir.store | MTE | 10 cycles | 128 bytes/cycle |
| hivm.hir.vadd | Vec | 5 cycles | 256 bytes/cycle |
| hivm.hir.vmul | Vec | 5 cycles | 256 bytes/cycle |
| hivm.hir.matmul | Cube | 20 cycles | 512 bytes/cycle |

**延迟计算**: `latency = max(baseLatency, dataSize / bytesPerCycle)`

## 输出示例

### 异步模式

```
Starting simulation (mode: asynchronous)...

PC[0] IsaID[0] arith.constant
PC[1] IsaID[1] arith.constant
PC[2] IsaID[2] for.init
PC[3] IsaID[3] for.condition -> true:4 false:8
PC[4] IsaID[4] arith.addi
PC[5] IsaID[5] for.increment
PC[6] IsaID[6] jump -> PC 3
PC[7] IsaID[7] func.return

Simulation completed.
Final PC: 8
Total cycles: 15
```

## 项目结构

```
ascendir_parser/
├── include/
│   ├── Isa.h                     # Isa 基类定义
│   ├── IsaRegistry.h             # 指令注册系统
│   ├── IsaSequence.h             # 指令序列管理
│   ├── IsaExecutor.h             # 指令执行器
│   ├── ExecutionContext.h        # 执行上下文（含cycle和多组件管理）
│   ├── ExecutionUnit.h           # 执行单元定义（MTE/Cube/Vec）
│   ├── Parser.h                  # MLIR解析器
│   └── Instructions/
│       ├── ArithIsas.h           # 算术指令类
│       ├── FuncIsas.h            # 函数指令类
│       ├── HivmIsas.h            # HIVM指令类
│       └── ControlFlowIsas.h     # 控制流指令类
│
├── src/
│   ├── main.cpp                  # 主程序
│   ├── Parser.cpp                # MLIR解析实现
│   ├── Analyzer.cpp              # MLIR未注册操作分析器
│   ├── ExecutionContext.cpp      # 运行时状态管理
│   ├── Isa.cpp                   # Isa 静态变量定义
│   ├── IsaRegistry.cpp           # 注册系统实现
│   ├── IsaSequence.cpp           # 序列构建实现
│   └── IsaExecutor.cpp           # 执行器实现
│
├── test/
│   ├── test_simple.mlir          # 简单测试
│   ├── test_arith.mlir           # 算术测试
│   ├── test_scf_for.mlir         # 循环测试
│   ├── test_nested_for.mlir      # 嵌套循环测试
│   ├── test_scf_if.mlir          # 条件分支测试
│   ├── test_hivm_basic.mlir      # HIVM方言测试
│   ├── test_multi_component.mlir # 多组件协作测试
│   ├── test_matmul_pipeline.mlir # 矩阵乘法流水线测试
│   └── test_pipeline.mlir        # 完整流水线测试
│
└── docs/
    ├── SimulatorDesign.md        # 设计文档
    ├── SimulatorUsage.md         # 使用说明
    └── SimulatorSummary.md       # 开发总结
```

## 架构特点

### 1. Isa 基类设计
- 所有指令继承自 Isa 基类
- 包含 `decode()` 方法：从 MLIR 操作解码
- 包含 `execute()` 方法：执行指令逻辑
- **IsaID**：全局唯一实例ID，每次创建指令时递增
- **PC**：程序计数器，标识指令在程序中的位置

### 2. IsaID vs PC 的区别
- **IsaID**：每次创建指令实例时分配，全局唯一且递增
- **PC**：指令在程序中的位置，同一指令多次执行时 PC 相同但 IsaID 不同
- 例如：for 循环中的指令，每次迭代 PC 相同，但 IsaID 递增

### 3. 统一指令管理
- 注册表模式支持动态扩展
- 易于添加新dialect
- 模块化的指令类
- 指令耗时配置

### 4. 类型安全
- 使用APInt/APFloat支持MLIR类型系统
- 正确处理SSA值
- 类型安全的值存储

### 5. 控制流展平
- scf.for → ForInit + ForCondition + ForIncrement
- scf.if → IfCondition + Jump
- 支持嵌套控制流

### 6. 多组件并行执行
- Scalar：主调度单元，负责标量运算和任务分发
- MTE：内存传输引擎，处理load/store
- Cube：矩阵计算单元，处理matmul
- Vec：向量计算单元，处理vadd/vmul
- 异步任务分发和同步等待机制

### 7. 数据大小相关延迟
- 根据数据量动态计算延迟
- 支持不同组件的吞吐量配置
- 真实模拟NPU硬件行为

## 扩展新指令

在 `include/Instructions/` 目录下添加新的指令类：

```cpp
// 1. 创建指令类
class MyCustomIsa : public Isa {
public:
    bool decode(mlir::Operation* op) override {
        return Isa::decode(op);
    }
    
    void execute(ExecutionContext& ctx) override {
        // 实现指令逻辑
    }
    
    std::string getDescription() const override {
        return "mydialect.myop";
    }
};

// 2. 注册指令（固定延迟）
REGISTER_ISA_WITH_LATENCY("mydialect.myop", MyCustomIsa, 10);

// 3. 注册指令到特定组件
REGISTER_ISA_ON_UNIT("mydialect.vecop", MyVecIsa, 5, ExecutionUnitType::Vec);

// 4. 注册数据大小相关延迟的指令
REGISTER_ISA_WITH_DATA_SIZE("mydialect.mteop", MyMteIsa,
                            calculateDataSize, model,
                            ExecutionUnitType::MTE);
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
