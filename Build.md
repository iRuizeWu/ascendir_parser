# Build Guide

## 1. 依赖分析方法

### 1.1 如何确定BiShengIR的依赖

当链接BiShengIR库时出现`undefined reference`错误，说明缺少依赖库。以下是分析方法：

#### 方法一：查看可用库列表

```bash
# 查看所有BiShengIR库
ls /home/ruize/code/github/AscendNPU-IR/build/lib/libBiShengIR*.a | xargs -I{} basename {} .a | sed 's/lib//'

# 查看所有MLIR库
ls /home/ruize/code/github/AscendNPU-IR/build/lib/libMLIR*.a | xargs -I{} basename {} .a | sed 's/lib//'
```

#### 方法二：根据错误信息定位

链接错误通常包含以下格式：
```
undefined reference to `mlir::xxx::XxxDialect::XxxDialect(mlir::MLIRContext*)'
```

根据`xxx`命名空间定位需要的库：
- `mlir::linalg::*` → `MLIRLinalgDialect`
- `mlir::scf::*` → `MLIRSCFDialect`
- `mlir::tensor::*` → `MLIRTensorDialect`
- `mlir::hivm::*` → `BiShengIRHIVMDialect`

#### 方法三：查看CMakeLists.txt

```bash
# 查看BiShengIR如何链接MLIR库
cat /home/ruize/code/github/AscendNPU-IR/bishengir/lib/Dialect/HIVM/IR/CMakeLists.txt
```

---

## 2. 常见报错及处理方法

### 2.1 编译期错误

#### 错误1：头文件找不到
```
fatal error: bishengir/Dialect/HIVM/IR/HIVM.h: No such file or directory
```

**原因**：缺少include路径

**解决方法**：
```cmake
# 在CMakeLists.txt中添加
include_directories(${BISHENGIR_SOURCE_DIR}/bishengir/include)
include_directories(${BISHENGIR_DIR}/tools/bishengir/bishengir/include)
```

#### 错误2：命名空间错误
```
error: 'memrefext' is not a member of 'mlir'
```

**原因**：命名空间不正确

**解决方法**：查看头文件确定正确命名空间
```bash
grep "namespace" /path/to/Dialect.h
```

BiShengIR的命名空间规则：
- HIVM: `mlir::hivm`
- HFusion: `mlir::hfusion`
- HACC: `mlir::hacc`
- MemRefExt: `bishengir::memref_ext` (注意不是mlir)

### 2.2 链接期错误

#### 错误1：undefined reference to TypeIDResolver
```
undefined reference to `mlir::detail::TypeIDResolver<mlir::math::MathDialect, void>::id'
```

**原因**：缺少对应Dialect的链接库

**解决方法**：添加对应的库
```cmake
target_link_libraries(ascendir_parser PRIVATE MLIRMathDialect)
```

#### 错误2：undefined reference to Dialect构造函数
```
undefined reference to `mlir::linalg::LinalgDialect::LinalgDialect(mlir::MLIRContext*)'
```

**原因**：缺少Dialect库

**解决方法**：
```cmake
target_link_libraries(ascendir_parser PRIVATE MLIRLinalgDialect)
```

#### 错误3：undefined reference to Op操作
```
undefined reference to `mlir::scf::ForOp::getRegionIterArgs()'
```

**原因**：Dialect有依赖其他库

**解决方法**：添加依赖库
```cmake
target_link_libraries(ascendir_parser PRIVATE MLIRSCFDialect)
```

---

## 3. CMake编译链条分析

### 3.1 当前构建方式

```
ascendir_parser/CMakeLists.txt
    │
    ├── find_package(LLVM)      # 查找LLVM
    ├── find_package(MLIR)      # 查找MLIR
    │
    ├── include(AddLLVM)        # LLVM构建工具
    ├── include(AddMLIR)        # MLIR构建工具
    │
    ├── include(BiShengIRDeps)  # BiShengIR依赖配置
    │
    └── add_subdirectory(src)
            │
            └── link_bishengir(ascendir_parser)  # 使用封装函数
```

### 3.2 依赖关系图

```
BiShengIRHIVMDialect
    ├── BiShengIRDialectUtils
    ├── BiShengIRHFusionDialect
    │       └── BiShengIRMathExtDialect
    ├── MLIRLinalgDialect
    │       └── MLIRTensorDialect
    ├── MLIRSCFDialect
    ├── MLIRMathDialect
    ├── MLIRAffineDialect
    └── MLIRBufferizationDialect
```

### 3.3 原有问题

不使用配置文件时需要手动添加所有依赖库，存在以下问题：

1. **维护困难**：每次添加新Dialect都需要手动添加链接库
2. **编译时间长**：链接不必要的库
3. **容易遗漏**：深层依赖不易发现

---

## 4. 优化方案：cmake/BiShengIRDeps.cmake

### 4.1 文件作用

`cmake/BiShengIRDeps.cmake` 是一个**依赖管理模块**，用于：

| 功能 | 说明 |
|------|------|
| 集中管理 | 所有依赖库列表集中在一个文件中 |
| 模块化 | 依赖配置与主CMakeLists.txt分离 |
| 可复用 | 提供封装函数，简化使用 |
| 易维护 | 修改依赖只需改一处 |

### 4.2 文件结构

```cmake
# cmake/BiShengIRDeps.cmake

# BiShengIR dialect库列表
set(BISHENGIR_DIALECT_LIBS
    BiShengIRHIVMDialect
    BiShengIRHFusionDialect
    BiShengIRHACCDialect
    ...  # 省略其他
    CACHE INTERNAL "BiShengIR dialect libraries"
)

# MLIR依赖库列表
set(MLIR_DIALECT_LIBS
    MLIRLinalgDialect
    MLIRSCFDialect
    MLIRTensorDialect
    ...  # 省略其他
    CACHE INTERNAL "MLIR dialect libraries required by BiShengIR"
)

# 封装函数：一键链接所有依赖
function(link_bishengir target)
    target_link_libraries(${target} PRIVATE
        ${BISHENGIR_DIALECT_LIBS}
        ${MLIR_DIALECT_LIBS}
    )
endfunction()
```

### 4.3 使用方法

**步骤1**：在主CMakeLists.txt中包含配置文件

```cmake
# 将cmake目录加入模块路径
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# 条件包含（仅当启用BiShengIR时）
if(BISHENGIR_DIR AND BISHENGIR_SOURCE_DIR)
  include(BiShengIRDeps)
endif()
```

**步骤2**：在src/CMakeLists.txt中使用封装函数

```cmake
if(BISHENGIR_DIR AND BISHENGIR_SOURCE_DIR)
  target_compile_definitions(ascendir_parser PRIVATE USE_BISHENGIR=1)
  link_bishengir(ascendir_parser)  # 一行搞定所有依赖
endif()
```

### 4.4 对比：使用前 vs 使用后

**使用前**（手动维护）：

```cmake
# src/CMakeLists.txt - 需要手动列出所有库
target_link_libraries(ascendir_parser PRIVATE
    BiShengIRHIVMDialect
    BiShengIRHFusionDialect
    BiShengIRHACCDialect
    BiShengIRAnnotationDialect
    BiShengIRMemRefExtDialect
    BiShengIRScopeDialect
    BiShengIRSymbolDialect
    BiShengIRTensorDialect
    BiShengIRMathExtDialect
    BiShengIRDialectUtils
    MLIRLinalgDialect
    MLIRSCFDialect
    MLIRTensorDialect
    MLIRMathDialect
    MLIRAffineDialect
    MLIRBufferizationDialect
    MLIRVectorDialect
    MLIRLLVMDialect
    # ... 可能遗漏某些依赖
)
```

**使用后**（模块化管理）：

```cmake
# src/CMakeLists.txt - 一行搞定
link_bishengir(ascendir_parser)
```

### 4.5 优势总结

| 方面 | 手动维护 | 使用BiShengIRDeps.cmake |
|------|----------|-------------------------|
| 代码量 | 多（每个目标重复） | 少（一行调用） |
| 维护性 | 差（分散多处） | 好（集中管理） |
| 可读性 | 低（大量库名） | 高（语义清晰） |
| 扩展性 | 差（需改多处） | 好（改一处即可） |
| 错误率 | 高（易遗漏） | 低（统一维护） |

### 4.6 扩展：如何添加新的依赖

当发现新的链接错误时：

1. 定位缺失的库名（根据错误信息）
2. 编辑 `cmake/BiShengIRDeps.cmake`
3. 将库名添加到对应的变量列表中

```cmake
# 例如发现缺少 MLIRGPUDialect
set(MLIR_DIALECT_LIBS
    MLIRLinalgDialect
    MLIRSCFDialect
    ...
    MLIRGPUDialect  # 新增
)
```

---

## 5. 常用调试命令

```bash
# 查看库依赖
ldd build/src/ascendir_parser

# 查看符号表
nm -C build/lib/libBiShengIRHIVMDialect.a | grep "Dialect"

# 查看编译命令
make VERBOSE=1

# 清理并重新构建
rm -rf build/* && cmake .. && make -j$(nproc)

# 查看BiShengIR库列表
ls /home/ruize/code/github/AscendNPU-IR/build/lib/libBiShengIR*.a | xargs -I{} basename {} .a | sed 's/lib//'

# 查看MLIR库列表
ls /home/ruize/code/github/AscendNPU-IR/build/lib/libMLIR*.a | xargs -I{} basename {} .a | sed 's/lib//'
```

## 6. 完整依赖列表

当前ascendir_parser项目需要的所有库：

| 类别 | 库名 | 用途 |
|------|------|------|
| MLIR基础 | MLIRIR, MLIRParser, MLIRSupport | 核心IR和解析 |
| MLIR Dialect | MLIRFuncDialect | func操作 |
| MLIR Dialect | MLIRArithDialect | 算术操作 |
| MLIR Dialect | MLIRMemRefDialect | 内存引用 |
| MLIR Dialect | MLIRLinalgDialect | 线性代数 |
| MLIR Dialect | MLIRSCFDialect | 控制流 |
| MLIR Dialect | MLIRTensorDialect | 张量操作 |
| MLIR Dialect | MLIRMathDialect | 数学函数 |
| BiShengIR | BiShengIRHIVMDialect | HIVM方言 |
| BiShengIR | BiShengIRHFusionDialect | HFusion方言 |
| BiShengIR | BiShengIRHACCDialect | HACC方言 |
| BiShengIR | BiShengIRDialectUtils | 工具函数 |

## 7. 项目文件结构

```
ascendir_parser/
├── CMakeLists.txt          # 主构建文件
├── cmake/
│   └── BiShengIRDeps.cmake # 依赖管理模块
├── include/
│   └── Parser.h            # 解析器API
├── src/
│   ├── CMakeLists.txt      # 源码构建配置
│   ├── main.cpp            # 主程序入口
│   └── Parser.cpp          # MLIR解析实现
├── Build.md                # 本文件
└── README.md               # 项目说明
```
