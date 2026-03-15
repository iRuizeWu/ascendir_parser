# MLIR仿真器开发总结

## 已完成功能

### 阶段1：基础框架 ✅

1. **核心数据结构**
   - [Isa.h](../include/Isa.h) - ISA 基类（含硬件特性描述）
   - [IsaSequence.h](../include/IsaSequence.h) - ISA 序列管理
   - [ExecutionContext.h](../include/ExecutionContext.h) - 执行上下文（值存储）
   - [IsaRegistry.h](../include/IsaRegistry.h) - ISA 注册表（简化版工厂模式）
   - [IsaExecutor.h](../include/IsaExecutor.h) - ISA 执行器（PC/Cycle 管理）

2. **实现文件**
   - [Isa.cpp](../src/Isa.cpp) - ISA 基类实现
   - [IsaRegistry.cpp](../src/IsaRegistry.cpp) - 注册表实现
   - [IsaSequence.cpp](../src/IsaSequence.cpp) - 序列构建和管理
   - [IsaExecutor.cpp](../src/IsaExecutor.cpp) - PC 驱动执行引擎
   - [ExecutionContext.cpp](../src/ExecutionContext.cpp) - 运行时状态管理

3. **指令类文件**
   - [ArithIsas.h](../include/Instructions/ArithIsas.h) - arith 指令类
   - [ControlFlowIsas.h](../include/Instructions/ControlFlowIsas.h) - 控制流指令类
   - [FuncIsas.h](../include/Instructions/FuncIsas.h) - func 指令类（含外部函数检测）
   - [HivmIsas.h](../include/Instructions/HivmIsas.h) - hivm 指令类
   - [YieldIsa.h](../include/Instructions/YieldIsa.h) - yield 赋值指令（YieldAssignIsa、IfYieldIsa）

4. **命令行接口**
   - `--simulate` - 执行 MLIR 程序
   - `--dump-sequence` - 显示指令序列
   - `--verbose` - 显示详细执行信息（含 cycle 耗时）
   - `--func-config <file>` - 指定外部函数配置文件
   - `--func-cycle <name>=<cycles>` - 设置函数延迟
   - `--dump-config` - 显示当前配置

### 阶段2：算术运算 ✅

1. **整数运算**
   - `arith.constant` - 整数常量
   - `arith.addi` - 加法 (1 cycle) - **支持不同位宽操作数**
   - `arith.subi` - 减法 (1 cycle) - **支持不同位宽操作数**
   - `arith.muli` - 乘法 (3 cycles) - **支持不同位宽操作数**
   - `arith.divsi` - 除法 (10 cycles)
   - `arith.cmpi` - 比较（支持所有 10 种谓词）
   - `arith.index_cast` - 类型转换 (5 cycles)

2. **浮点运算**
   - `arith.constant` - 浮点常量
   - `arith.addf` - 加法 (2 cycles)
   - `arith.subf` - 减法 (2 cycles)
   - `arith.mulf` - 乘法 (4 cycles)
   - `arith.divf` - 除法 (12 cycles)

3. **函数操作**
   - `func.call` - 函数调用 (5 cycles)
   - `func.return` - 函数返回 (1 cycle)

4. **位宽一致性处理** ✅
   - 整数运算指令自动处理不同位宽操作数
   - 使用符号扩展（sext）统一操作数位宽
   - 确保APInt运算的位宽一致性
   - 支持index类型与i32/i64等类型的混合运算

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
   - `hivm.hir.load` - GM到UB数据加载 (MTE, 动态延迟)
   - `hivm.hir.vadd` - 向量加法 (Vec, 动态延迟)
   - `hivm.hir.vmul` - 向量乘法 (Vec, 动态延迟)
   - `hivm.hir.matmul` - 矩阵乘法 (Cube, 动态延迟)
   - `hivm.hir.store` - UB到GM数据存储 (MTE, 动态延迟)

3. **数据类型支持**
   - f16 半精度浮点
   - memref 多维数组

### 阶段6：指令耗时模拟 ✅

1. **Isa 类自描述硬件特性**
   - `getExecutionUnit()` - 返回执行单元类型
   - `getLatency()` - 返回基础延迟
   - `getHardwareCharacteristics()` - 返回完整硬件特性（可动态计算）

2. **Cycle计数**
   - IsaExecutor 包含 cycle 计数器
   - 每次 tick() 递增 cycle

3. **输出格式**
   - `Cycle[X] PC[Y] IsaID[Z] ...` - 执行信息
   - `Total cycles: N` - 总耗时统计

### 阶段7：多组件并行执行 ✅

1. **执行单元类型**
   - [ExecutionUnit.h](../include/ExecutionUnit.h) - 执行单元定义
   - Scalar：标量单元，负责调度和标量运算
   - MTE：内存传输引擎，处理load/store
   - Cube：矩阵计算单元，处理matmul
   - Vec：向量计算单元，处理vadd/vmul

2. **Isa 类硬件特性描述**
   - 每个 Isa 子类定义自己的执行单元和延迟
   - 支持动态延迟计算（根据数据大小）

3. **异步任务管理**
   - PendingTask：待处理任务结构
   - 任务分发和完成跟踪
   - 同步等待机制

4. **执行模式**
   - 异步模式（默认）：Scalar分发任务后继续执行
   - 同步模式：每条指令等待完成后才继续
   - `--sync` 命令行选项

### 阶段8：外部函数配置 ✅

1. **配置系统**
   - [ExternalFunctionConfig.h](../include/ExternalFunctionConfig.h) - 外部函数配置类
   - [ExternalFunctionConfig.cpp](../src/ExternalFunctionConfig.cpp) - 配置解析实现
   - [config/external_func.yml](../config/external_func.yml) - YAML配置文件

2. **外部函数检测**
   - 自动检测只有声明没有实现的函数
   - 通过配置文件指定延迟和执行单元
   - 支持命令行动态设置

3. **命令行选项**
   - `--func-config <file>` - 指定配置文件
   - `--func-cycle <name>=<cycles>` - 设置函数延迟
   - `--dump-config` - 显示当前配置

### 阶段9：增强的Yield处理 ✅

1. **YieldAssignIsa**
   - 处理scf.for中的yield
   - 支持位置信息显示（scf.for.yield）
   - 支持值显示（操作结果 -> 目标参数）

2. **IfYieldIsa**
   - 处理scf.if中的yield
   - 支持位置信息显示（scf.if.then / scf.if.else）
   - 支持值显示（操作结果）

3. **值缓存机制**
   - execute()方法中缓存计算值
   - getDescription()方法中显示缓存的值

## 测试验证

所有测试用例均已通过（共19项）：

### 基础功能测试
- ✅ [test_simple.mlir](../test/test_simple.mlir) - 简单常量测试
- ✅ [test_arith.mlir](../test/test_arith.mlir) - 算术运算测试
- ✅ [test_compute.mlir](../test/test_compute.mlir) - 复杂计算测试
- ✅ [test_float.mlir](../test/test_float.mlir) - 浮点运算测试
- ✅ [test_compare.mlir](../test/test_compare.mlir) - 比较运算测试
- ✅ [test_all.mlir](../test/test_all.mlir) - 综合测试

### 控制流测试
- ✅ [test_scf_for.mlir](../test/test_scf_for.mlir) - 循环测试
- ✅ [test_scf_if.mlir](../test/test_scf_if.mlir) - 条件分支测试
- ✅ [test_scf_if_else.mlir](../test/test_scf_if_else.mlir) - if-else分支测试
- ✅ [test_nested.mlir](../test/test_nested.mlir) - 嵌套控制流测试（包含位宽转换）
- ✅ [test_nested_for.mlir](../test/test_nested_for.mlir) - 嵌套循环测试
- ✅ [test_nested_if.mlir](../test/test_nested_if.mlir) - 嵌套条件测试
- ✅ [test_complex_control_flow.mlir](../test/test_complex_control_flow.mlir) - 复杂控制流测试
- ✅ [test_deep_nesting.mlir](../test/test_deep_nesting.mlir) - 深度嵌套测试

### 高级功能测试
- ✅ [test_pipeline.mlir](../test/test_pipeline.mlir) - 流水线测试
- ✅ [test_matmul_pipeline.mlir](../test/test_matmul_pipeline.mlir) - 矩阵乘法流水线测试
- ✅ [test_multi_component.mlir](../test/test_multi_component.mlir) - 多组件协作测试

### HIVM方言测试
- ✅ [test_hivm_basic.mlir](../test/test_hivm_basic.mlir) - HIVM基础测试
- ✅ [test_hivm_complex.mlir](../test/test_hivm_complex.mlir) - HIVM复杂测试

## 架构特点

### 1. PC驱动执行模型
- IsaExecutor 管理 PC 和 Cycle
- tick() 方法驱动一次时钟周期
- 简单清晰的执行流程

### 2. Isa 类自描述硬件特性
- `getExecutionUnit()` - 执行单元类型
- `getLatency()` - 基础延迟
- `getHardwareCharacteristics()` - 完整硬件特性

### 3. 职责分离

| 组件 | 职责 |
|------|------|
| ExecutionContext | 值存储、调用栈管理 |
| IsaExecutor | PC/Cycle 管理、执行单元实例化、阻塞管理 |
| IsaRegistry | 创建 Isa 实例（工厂模式） |
| Isa 类 | 执行逻辑 + 硬件特性描述 |

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

### 输出示例

```
Simulation completed.
Total cycles: 56

## 技术亮点

1. **模块化设计** - 各组件职责明确，易于维护和扩展
2. **Isa 自描述** - 每个指令类定义自己的硬件特性
3. **类型安全** - 正确处理MLIR类型系统，支持不同位宽操作数运算
4. **渐进式实现** - 分阶段实现，降低开发风险
5. **指令耗时模拟** - 支持cycle级别的性能分析
6. **多组件并行执行** - 真实模拟NPU硬件行为
7. **数据大小相关延迟** - 根据数据量动态计算延迟
8. **异步/同步模式切换** - 灵活的执行模式选择
9. **位宽一致性处理** - 自动处理不同位宽整数运算，避免APInt断言错误
10. **外部函数配置** - YAML配置文件支持自定义延迟和执行单元
11. **增强的Yield处理** - 支持位置信息和值显示，便于调试

## 文档

- [SimulatorDesign.md](./SimulatorDesign.md) - 设计文档
- [SimulatorUsage.md](./SimulatorUsage.md) - 使用说明

## 总结

MLIR仿真器已完成全部9个阶段的开发，支持arith、func、scf、memref和hivm dialect。系统架构清晰，易于扩展，支持控制流展平、指令耗时模拟、多组件并行执行、外部函数配置和增强的yield处理，为后续的指令发射优化提供了良好的基础。

### 核心能力

1. **PC驱动执行** - 简单清晰的执行模型
2. **控制流展平** - 支持scf.for和scf.if
3. **Isa 自描述硬件特性** - 每个指令类定义自己的执行单元和延迟
4. **指令耗时模拟** - cycle级别性能分析
5. **多组件并行执行** - Scalar、MTE、Cube、Vec组件协作
6. **数据大小相关延迟** - 真实模拟NPU硬件行为
7. **灵活的执行模式** - 异步/同步模式切换
8. **外部函数配置** - YAML配置文件支持自定义延迟和执行单元
9. **增强的Yield处理** - 支持位置信息和值显示

