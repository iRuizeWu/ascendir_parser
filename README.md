# ascendir_parser

MLIR文件解析器，支持解析和打印MLIR IR，包括BiShengIR dialect。

## 功能特性

- 解析MLIR文件并打印IR
- 支持`--dump`选项，显示函数入口和IR源字符串
- 支持标准MLIR dialect（func, arith, memref等）
- 支持BiShengIR dialect（hivm, hfusion, hacc等）
- 分析未注册操作（intr、hivm.intr等），提取属性、操作数、结果等详细信息

## 目录结构

```
ascendir_parser/
├── CMakeLists.txt          # 主构建文件
├── cmake/
│   └── BiShengIRDeps.cmake # 依赖管理模块（集中管理所有依赖库）
├── include/
│   └── Parser.h            # 解析器API
├── src/
│   ├── CMakeLists.txt      # 源码构建配置
│   ├── main.cpp            # 主程序入口
│   ├── Parser.cpp          # MLIR解析实现
│   └── Analyzer.cpp        # MLIR未注册操作分析器
├── build/                  # 构建目录（生成）
├── Build.md                # 构建指南和依赖分析
└── README.md               # 本文件
```

## 依赖管理说明

### cmake/BiShengIRDeps.cmake

`cmake/BiShengIRDeps.cmake` 是项目的依赖管理模块，提供以下功能：

- **集中管理**：所有BiShengIR和MLIR依赖库列表集中在一个文件中
- **封装函数**：提供 `link_bishengir(target)` 函数，一键链接所有依赖
- **易于维护**：修改依赖只需修改一处

详细说明请参考 [Build.md](Build.md#4-优化方案cmakebishengirdepscmake)

---

## 依赖

- LLVM 19+
- MLIR
- BiShengIR（可选，用于HIVM dialect支持）

## 快速开始

### 1. 基础编译（不含BiShengIR）

```bash
cd /home/ruize/code/github/ascendir_parser

mkdir -p build && cd build

cmake -G Ninja .. \
    -DLLVM_DIR=/home/ruize/code/github/AscendNPU-IR/build \
    -DMLIR_DIR=/home/ruize/code/github/AscendNPU-IR/build/lib/cmake/mlir

cmake --build . -j$(nproc)
```

### 2. 完整编译（含BiShengIR支持）

```bash
cd /home/ruize/code/github/ascendir_parser

mkdir -p build && cd build

cmake -G Ninja .. \
    -DLLVM_DIR=/home/ruize/code/github/AscendNPU-IR/build \
    -DMLIR_DIR=/home/ruize/code/github/AscendNPU-IR/build/lib/cmake/mlir \
    -DBISHENGIR_DIR=/home/ruize/code/github/AscendNPU-IR/build \
    -DBISHENGIR_SOURCE_DIR=/home/ruize/code/github/AscendNPU-IR

cmake --build . -j$(nproc)
```

### 3. 重新构建（清理后）

```bash
cd /home/ruize/code/github/ascendir_parser/build
rm -rf *
cmake -G Ninja .. \
    -DLLVM_DIR=/home/ruize/code/github/AscendNPU-IR/build \
    -DMLIR_DIR=/home/ruize/code/github/AscendNPU-IR/build/lib/cmake/mlir \
    -DBISHENGIR_DIR=/home/ruize/code/github/AscendNPU-IR/build \
    -DBISHENGIR_SOURCE_DIR=/home/ruize/code/github/AscendNPU-IR
cmake --build . -j$(nproc)
```

### 4. 一键编译脚本

```bash
# 基础版
cd /home/ruize/code/github/ascendir_parser && rm -rf build/* && \
cmake -G Ninja -S . -B build \
    -DLLVM_DIR=/home/ruize/code/github/AscendNPU-IR/build \
    -DMLIR_DIR=/home/ruize/code/github/AscendNPU-IR/build/lib/cmake/mlir && \
cmake --build build -j$(nproc)

# 完整版
cd /home/ruize/code/github/ascendir_parser && rm -rf build/* && \
cmake -G Ninja -S . -B build \
    -DLLVM_DIR=/home/ruize/code/github/AscendNPU-IR/build \
    -DMLIR_DIR=/home/ruize/code/github/AscendNPU-IR/build/lib/cmake/mlir \
    -DBISHENGIR_DIR=/home/ruize/code/github/AscendNPU-IR/build \
    -DBISHENGIR_SOURCE_DIR=/home/ruize/code/github/AscendNPU-IR && \
cmake --build build -j$(nproc)
```

---


cmake/BiShengIRDeps.cmake 作用
# 集中管理依赖库列表
set(BISHENGIR_DIALECT_LIBS ...)  # BiShengIR库
set(MLIR_DIALECT_LIBS ...)       # MLIR依赖库
# 封装函数：一行链接所有依赖
function(link_bishengir target)
    target_link_libraries(${target} PRIVATE ${BISHENGIR_DIALECT_LIBS} ${MLIR_DIALECT_LIBS})
endfunction()

## 使用方法

### 基础用法

```bash
./build/src/ascendir_parser <mlir-file>
```

### 显示帮助

```bash
./build/src/ascendir_parser --help
```

输出：
```
Usage: ascendir_parser [options] <mlir-file>
Options:
  --dump           Print function entries and IR source strings
  --simulate       Run MLIR simulation
  --dump-sequence  Dump instruction sequence (with --simulate)
  --verbose        Show detailed execution info (with --simulate)
  --sync           Synchronous execution mode (with --simulate)
  --help           Show this help message
```

### 解析并打印IR

```bash
./build/src/ascendir_parser test.mlir
```

### 使用 --dump 选项

```bash
./build/src/ascendir_parser --dump test_hivm.mlir
```

输出示例：
```
=== Function Entries ===
Function: mul_add_mix_aic
Function: mul_add_mix_aiv

=== IR Source Strings ===
--- mul_add_mix_aic ---
func.func @mul_add_mix_aic(%arg0: memref<64x64xf16>, ...) {
  hivm.hir.matmul ins(%arg1, %arg2 : ...) outs(%arg3 : ...)
  return
}

--- mul_add_mix_aiv ---
...
```

### 运行仿真器

```bash
# 基本仿真
./build/src/ascendir_parser test/test_simple.mlir --simulate

# 显示指令序列
./build/src/ascendir_parser test/test_for.mlir --simulate --dump-sequence

# 详细执行信息（含cycle耗时）
./build/src/ascendir_parser test/test_pipeline.mlir --simulate --verbose

# 同步模式（对比异步模式）
./build/src/ascendir_parser test/test_pipeline.mlir --simulate --verbose --sync
```

---

## 测试文件

### 解析器测试

| 文件 | 说明 |
|------|------|
| test.mlir | 简单func测试 |
| test2.mlir | arith dialect测试 |
| test_memref.mlir | memref dialect测试 |
| test_hivm.mlir | BiShengIR HIVM dialect测试 |
| test_llvm_hivm_intr.mlir | LLVM + HIVM intrinsic测试 |
| test_with_branch.mlir | 控制流分支测试 |

### 仿真器测试

| 文件 | 说明 |
|------|------|
| test/test_simple.mlir | 简单常量测试 |
| test/test_arith.mlir | 算术运算测试 |
| test/test_float.mlir | 浮点运算测试 |
| test/test_compare.mlir | 比较运算测试 |
| test/test_scf_for.mlir | 循环测试 |
| test/test_nested_for.mlir | 嵌套循环测试 |
| test/test_scf_if.mlir | 条件分支测试 |
| test/test_scf_if_else.mlir | if-else分支测试 |
| test/test_nested_if.mlir | 嵌套条件测试 |
| test/test_hivm_basic.mlir | HIVM方言测试 |
| test/test_multi_component.mlir | 多组件协作测试 |
| test/test_pipeline.mlir | 完整流水线测试 |

### 运行所有测试

```bash
# 进入项目目录
cd /home/ruize/code/github/ascendir_parser

# 解析器测试
./build/src/ascendir_parser test.mlir
./build/src/ascendir_parser --dump test2.mlir
./build/src/ascendir_parser --dump test_memref.mlir

# BiShengIR测试（需要完整编译）
./build/src/ascendir_parser --dump test_hivm.mlir

# 仿真器测试
./build/src/ascendir_parser test/test_simple.mlir --simulate
./build/src/ascendir_parser test/test_for.mlir --simulate --verbose
./build/src/ascendir_parser test/test_pipeline.mlir --simulate --verbose
```

---

## CMake选项

| 选项 | 说明 | 必需 |
|------|------|------|
| `-DLLVM_DIR` | LLVM构建目录 | 是 |
| `-DMLIR_DIR` | MLIR cmake目录 | 是 |
| `-DBISHENGIR_DIR` | BiShengIR构建目录 | 否（启用BiShengIR需要） |
| `-DBISHENGIR_SOURCE_DIR` | BiShengIR源码目录 | 否（启用BiShengIR需要） |

---

## 常见问题

### 找不到LLVM

```bash
# 确保LLVM已构建
ls /home/ruize/code/github/AscendNPU-IR/build/lib/cmake/llvm/LLVMConfig.cmake
```

### 链接错误

参考 [Build.md](Build.md) 中的依赖分析方法。

### 如何查看可用库

```bash
# BiShengIR库
ls /home/ruize/code/github/AscendNPU-IR/build/lib/libBiShengIR*.a

# MLIR库
ls /home/ruize/code/github/AscendNPU-IR/build/lib/libMLIR*.a
```

### 如何添加新的依赖

编辑 `cmake/BiShengIRDeps.cmake`，将缺少的库添加到对应变量中：

```cmake
set(MLIR_DIALECT_LIBS
    ...
    MLIRNewDialect  # 新增
)
```

---

## 构建时间

- 基础版（无BiShengIR，使用Ninja）：约5秒
- 完整版（含BiShengIR，使用Ninja）：约10秒

---

## 更多信息

- 详细构建指南、依赖分析和优化方案：[Build.md](Build.md)
- BiShengIR项目：`/home/ruize/code/github/AscendNPU-IR`
