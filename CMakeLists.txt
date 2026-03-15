cmake_minimum_required(VERSION 3.28.0)
project(ascendir_parser LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 设置默认构建类型为Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

# 优化选项
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os -DNDEBUG")

# 启用链接时优化
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
if(ipo_supported)
  message(STATUS "IPO/LTO enabled")
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
  message(STATUS "IPO/LTO not supported: ${ipo_error}")
endif()

# 移除不必要的符号
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s")

set(LLVM_DIR "" CACHE PATH "LLVM install directory (contains lib/cmake/llvm)")
set(MLIR_DIR "" CACHE PATH "MLIR install directory (contains lib/cmake/mlir)")
set(BISHENGIR_DIR "" CACHE PATH "BiShengIR build/install directory")
set(BISHENGIR_SOURCE_DIR "" CACHE PATH "BiShengIR source directory")

if(NOT LLVM_DIR)
  message(FATAL_ERROR "LLVM_DIR must be set to LLVM install directory")
endif()
if(NOT MLIR_DIR)
  set(MLIR_DIR "${LLVM_DIR}/lib/cmake/mlir")
endif()

find_package(LLVM REQUIRED CONFIG PATHS "${LLVM_DIR}/lib/cmake/llvm" NO_DEFAULT_PATH)
find_package(MLIR REQUIRED CONFIG PATHS "${MLIR_DIR}" NO_DEFAULT_PATH)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")

list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(AddLLVM)
include(TableGen)
include(AddMLIR)

set(ascendir_parser_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ascendir_parser_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(ascendir_parser_INCLUDE_DIR ${ascendir_parser_SRC_DIR}/include)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${MLIR_INCLUDE_DIRS})
include_directories(${ascendir_parser_INCLUDE_DIR})

if(BISHENGIR_DIR)
  set(BISHENGIR_INCLUDE_DIR "${BISHENGIR_DIR}/include")
  set(BISHENGIR_LIB_DIR "${BISHENGIR_DIR}/lib")
  include_directories(${BISHENGIR_INCLUDE_DIR})
  link_directories(${BISHENGIR_LIB_DIR})
  message(STATUS "BiShengIR include: ${BISHENGIR_INCLUDE_DIR}")
  message(STATUS "BiShengIR lib: ${BISHENGIR_LIB_DIR}")
endif()

if(BISHENGIR_SOURCE_DIR)
  include_directories(${BISHENGIR_SOURCE_DIR}/bishengir/include)
  include_directories(${BISHENGIR_DIR}/tools/bishengir/bishengir/include)
  message(STATUS "BiShengIR source include: ${BISHENGIR_SOURCE_DIR}/bishengir/include")
  message(STATUS "BiShengIR build include: ${BISHENGIR_DIR}/tools/bishengir/bishengir/include")
endif()

if(BISHENGIR_DIR AND BISHENGIR_SOURCE_DIR)
  include(BiShengIRDeps)
endif()

add_subdirectory(src)

# Google Test
enable_testing()

# Try to find GTest first
find_package(GTest QUIET CONFIG)

if(NOT GTest_FOUND)
  message(STATUS "GTest not found via find_package, trying FetchContent...")
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
else()
  message(STATUS "Found GTest: ${GTest_DIR}")
endif()

add_subdirectory(tests)
