# Ascendir Parser Unit Tests

本目录包含使用 Google Test 框架编写的单元测试，覆盖项目的三个最核心的组件。

## 测试覆盖

### 1. Parser (`test_parser.cpp`)
测试 MLIR 文件解析功能：
- **ParseSimpleModule**: 解析基本的 MLIR 模块
- **ExtractFunctions**: 从模块中提取函数信息
- **ParseArithmeticOps**: 解析包含算术运算的代码
- **ParseControlFlow**: 解析控制流操作

### 2. IsaSequence (`test_isa_sequence.cpp`)
测试指令序列管理：
- **AddInstruction**: 添加单个指令
- **AddMultipleInstructions**: 添加多个指令
- **AllocatePC**: PC 地址分配
- **LabelManagement**: 标签管理
- **GetInstructionByPC**: 通过 PC 检索指令
- **IsValidPC**: PC 有效性检查
- **UpdateJumpTarget**: 更新跳转目标
- **UpdateConditionalJump**: 更新条件跳转

### 3. IsaExecutor (`test_isa_executor.cpp`)
测试指令执行器：
- **Initialization**: 执行器初始化状态
- **PCManagement**: PC 寄存器管理
- **CycleCounter**: 周期计数器
- **HaltState**: 停止状态检查
- **BlockedState**: 阻塞状态检查
- **ExecutionUnitStatus**: 执行单元状态（Scalar、MTE、Cube、Vec）
- **VerboseMode/AsyncMode**: 运行模式设置
- **ContextAccess**: 上下文访问

## 构建和运行

### 前置条件
- CMake 3.28+
- LLVM 和 MLIR 工具链已安装
- Google Test（自动下载，如果系统未安装）

### 快速开始

```bash
# 设置 LLVM 环境变量
export LLVM_DIR=/path/to/llvm/install

# 执行测试脚本
cd tests
bash run_unit_tests.sh
```

### 手动构建

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DLLVM_DIR=$LLVM_DIR ..
make ascendir_tests
ctest --output-on-failure
```

### 运行特定测试

```bash
# 列出所有测试
./build/ascendir_tests --gtest_list_tests

# 运行特定测试
./build/ascendir_tests --gtest_filter=ParserTest.ParseSimpleModule

# 运行特定测试类
./build/ascendir_tests --gtest_filter=IsaSequenceTest.*

# 详细输出
./build/ascendir_tests --gtest_filter=* --gtest_print_time=1
```

## 扩展测试

要添加新的单元测试：

1. 在相应的测试文件中添加测试用例
2. 遵循 Google Test 命名约定：`TEST_F(TestClassName, TestName)`
3. 使用 `EXPECT_*` 或 `ASSERT_*` 宏进行断言
4. 重新构建项目

## 测试输出示例

```
[==========] Running 25 tests from 3 test suites.
[----------] Global test environment set-up.
[----------] 4 tests from ParserTest
[ RUN      ] ParserTest.ParseSimpleModule
[       OK ] ParserTest.ParseSimpleModule (15 ms)
[ RUN      ] ParserTest.ExtractFunctions
[       OK ] ParserTest.ExtractFunctions (8 ms)
...
[==========] 25 tests from 3 test suites ran.
[  PASSED  ] 25 tests.
```

## 常见问题

### 找不到 GoogleTest
Google Test 会通过 FetchContent 自动下载。如果连接失败，可以手动安装：
```bash
apt-get install libgtest-dev  # Ubuntu/Debian
```

### LLVM_DIR 未设置错误
确保在构建前设置了 LLVM 环境变量：
```bash
export LLVM_DIR=$(llvm-config --prefix)
export MLIR_DIR=$LLVM_DIR/lib/cmake/mlir
```

### 链接错误
如果遇到链接错误，确保所有必要的 LLVM/MLIR 库都已正确链接。检查 CMakeLists.txt 中的 `target_link_libraries` 是否包含所有必要的库。
