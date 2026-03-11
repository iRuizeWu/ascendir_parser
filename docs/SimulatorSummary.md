# MLIR仿真器开发总结

## 已完成功能

### 阶段1：基础框架 ✅

1. **核心数据结构**
   - [Instruction.h](../include/Instruction.h) - 统一指令表示
   - [InstructionSequence.h](../include/InstructionSequence.h) - 指令序列管理
   - [ExecutionContext.h](../include/ExecutionContext.h) - 执行上下文（含cycle计数）
   - [InstructionRegistry.h](../include/InstructionRegistry.h) - 指令注册表（含耗时配置）
   - [InstructionExecutor.h](../include/InstructionExecutor.h) - 指令执行器

2. **实现文件**
   - [InstructionSequence.cpp](../src/InstructionSequence.cpp) - 序列构建和管理
   - [ExecutionContext.cpp](../src/ExecutionContext.cpp) - 运行时状态管理
   - [InstructionRegistry.cpp](../src/InstructionRegistry.cpp) - 注册表实现
   - [InstructionExecutor.cpp](../src/InstructionExecutor.cpp) - PC驱动执行引擎

3. **命令行接口**
   - `--simulate` - 执行MLIR程序
   - `--dump-sequence` - 显示指令序列
   - `--verbose` - 显示详细执行信息（含cycle耗时）

### 阶段2：算术运算 ✅

1. **整数运算**
   - `arith.constant` - 整数常量
   - `arith.addi` - 加法 (1 cycle)
   - `arith.subi` - 减法 (1 cycle)
   - `arith.muli` - 乘法 (3 cycles)
   - `arith.divsi` - 除法 (10 cycles)
   - `arith.cmpi` - 比较（支持所有10种谓词）
   - `arith.index_cast` - 类型转换

2. **浮点运算**
   - `arith.constant` - 浮点常量
   - `arith.addf` - 加法 (2 cycles)
   - `arith.subf` - 减法 (2 cycles)
   - `arith.mulf` - 乘法 (4 cycles)
   - `arith.divf` - 除法 (12 cycles)

3. **函数操作**
   - `func.return` - 函数返回 (1 cycle)

### 阶段3：控制流展平 ✅

1. **scf.for 展平**
   - ForInit: 初始化循环变量和迭代参数
   - ForCondition: 检查循环条件 (iv < ub)
   - ForIncrement: 更新循环变量 (iv += step)
   - 支持嵌套循环
   - 支持 iter_args

2. **scf.if 展平**
   - IfCondition: 条件判断和跳转
   - 支持else分支
   - 支持嵌套if

### 阶段4：函数调用 ✅

1. **函数操作**
   - `func.call` - 函数调用 (5 cycles)
   - `func.return` - 函数返回 (1 cycle)
   - 调用栈管理
   - 参数传递

### 阶段5：HIVM方言 ✅

1. **内存操作**
   - `memref.alloc` - 内存分配 (1 cycle)

2. **HIVM操作**
   - `hivm.hir.load` - GM到UB数据加载 (100 cycles)
   - `hivm.hir.vadd` - 向量加法 (50 cycles)
   - `hivm.hir.store` - UB到GM数据存储 (100 cycles)

3. **数据类型支持**
   - f16 半精度浮点
   - memref 多维数组

### 阶段6：指令耗时模拟 ✅

1. **耗时配置**
   - InstructionInfo 结构体包含 handler 和 latency
   - REGISTER_INSTRUCTION_WITH_LATENCY 宏支持配置耗时

2. **Cycle计数**
   - ExecutionContext 包含 cycle 计数器
   - 每条指令执行后更新 cycle

3. **输出格式**
   - `[cycle=X] PC Y: ...` - 开始执行
   - `[cycle=X] retired` - 执行完成
   - `Total cycles: N` - 总耗时统计

### 阶段7：多组件并行执行 ✅

1. **执行单元类型**
   - [ExecutionUnit.h](../include/ExecutionUnit.h) - 执行单元定义
   - Scalar：标量单元，负责调度和标量运算
   - MTE：内存传输引擎，处理load/store
   - Cube：矩阵计算单元，处理matmul
   - Vec：向量计算单元，处理vadd/vmul

2. **组件延迟模型**
   - ComponentLatencyModel：支持数据大小相关延迟
   - MTE: base=10, 128 bytes/cycle
   - Vec: base=5, 256 bytes/cycle
   - Cube: base=20, 512 bytes/cycle

3. **异步任务管理**
   - PendingTask：待处理任务结构
   - 任务分发和完成跟踪
   - 同步等待机制

4. **执行模式**
   - 异步模式（默认）：Scalar分发任务后继续执行
   - 同步模式：每条指令等待完成后才继续
   - `--sync` 命令行选项

## 测试验证

所有测试用例均已通过：

- ✅ [test_simple.mlir](../test/test_simple.mlir) - 简单常量测试
- ✅ [test_arith.mlir](../test/test_arith.mlir) - 算术运算测试
- ✅ [test_compute.mlir](../test/test_compute.mlir) - 复杂计算测试
- ✅ [test_float.mlir](../test/test_float.mlir) - 浮点运算测试
- ✅ [test_compare.mlir](../test/test_compare.mlir) - 比较运算测试
- ✅ [test_all.mlir](../test/test_all.mlir) - 综合测试
- ✅ [test_for.mlir](../test/test_for.mlir) - 循环测试
- ✅ [test_nested_for.mlir](../test/test_nested_for.mlir) - 嵌套循环测试
- ✅ [test_if.mlir](../test/test_if.mlir) - 条件分支测试
- ✅ [test_nested_if.mlir](../test/test_nested_if.mlir) - 嵌套条件测试
- ✅ [test_call.mlir](../test/test_call.mlir) - 函数调用测试
- ✅ [test_hivm_basic.mlir](../test/test_hivm_basic.mlir) - HIVM方言测试
- ✅ [test_multi_component.mlir](../test/test_multi_component.mlir) - 多组件协作测试
- ✅ [test_matmul_pipeline.mlir](../test/test_matmul_pipeline.mlir) - 矩阵乘法流水线测试
- ✅ [test_pipeline.mlir](../test/test_pipeline.mlir) - 完整流水线测试

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

### 5. 多组件并行执行
- Scalar：主调度单元，负责标量运算和任务分发
- MTE：内存传输引擎，处理load/store
- Cube：矩阵计算单元，处理matmul
- Vec：向量计算单元，处理vadd/vmul
- 异步任务分发和同步等待机制

### 6. 数据大小相关延迟
- 根据数据量动态计算延迟
- 支持不同组件的吞吐量配置
- 真实模拟NPU硬件行为

## 使用示例

```bash
# 编译项目
cd /home/ruize/code/github/ascendir_parser
cmake --build build -j4

# 运行仿真（异步模式，含详细输出）
./build/src/ascendir_parser test/test_pipeline.mlir --simulate --verbose

# 运行仿真（同步模式，对比）
./build/src/ascendir_parser test/test_pipeline.mlir --simulate --verbose --sync
```

### 异步模式输出示例

```
Starting simulation (mode: asynchronous)...

[cycle=0] PC 0: Normal: %ubA = memref.alloc() : memref<128xf16>
  [memref.alloc] Allocated 256 bytes for memref<128xf16>
[cycle=1] Scalar advanced 1 cycles

[cycle=1] PC 1: Normal: "hivm.hir.load"(%arg0, %ubA) : ...
  [hivm.hir.load] GM -> UB on MTE
    Data size: 256 bytes (128 elements)
  [Dispatch] Task to MTE, duration=10 cycles, dataSize=256 bytes
  Active tasks:
    - hivm.hir.load on MTE (complete at cycle 11)

...

Simulation completed.
Final PC: 12
Total cycles: 56
All units idle: yes
```

### 同步模式输出示例

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
Final PC: 12
Total cycles: 326
All units idle: yes
```

## 技术亮点

1. **模块化设计** - 各组件职责明确，易于维护和扩展
2. **注册表模式** - 支持动态注册指令，扩展性强
3. **类型安全** - 正确处理MLIR类型系统
4. **渐进式实现** - 分阶段实现，降低开发风险
5. **指令耗时模拟** - 支持cycle级别的性能分析
6. **多组件并行执行** - 真实模拟NPU硬件行为
7. **数据大小相关延迟** - 根据数据量动态计算延迟
8. **异步/同步模式切换** - 灵活的执行模式选择

## 性能考虑

当前实现已经考虑了以下优化方向：
- 使用map存储值，支持快速查找
- 指令序列使用vector存储，支持快速索引
- PC到索引的映射，支持O(1)查找
- 指令耗时模拟，支持性能分析
- 多组件并行执行，提高吞吐量
- 数据大小相关延迟，真实模拟硬件

未来可以进一步优化：
- 指令缓存
- JIT编译
- 依赖分析和乱序执行
- 更细粒度的流水线模拟

## 文档

- [SimulatorDesign.md](./SimulatorDesign.md) - 设计文档
- [SimulatorUsage.md](./SimulatorUsage.md) - 使用说明

## 总结

MLIR仿真器已完成全部7个阶段的开发，支持arith、func、scf、memref和hivm dialect。系统架构清晰，易于扩展，支持控制流展平、指令耗时模拟和多组件并行执行，为后续的指令发射优化提供了良好的基础。

### 核心能力

1. **PC驱动执行** - 简单清晰的执行模型
2. **控制流展平** - 支持scf.for和scf.if
3. **指令耗时模拟** - cycle级别性能分析
4. **多组件并行执行** - Scalar、MTE、Cube、Vec组件协作
5. **数据大小相关延迟** - 真实模拟NPU硬件行为
6. **灵活的执行模式** - 异步/同步模式切换
