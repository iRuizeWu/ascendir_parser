#!/bin/bash

echo "=== MLIR仿真器测试脚本 ==="
echo ""

PASSED=0
FAILED=0

run_test() {
    local test_name=$1
    local test_file=$2
    
    echo "测试: $test_name"
    if ./build/src/ascendir_parser "$test_file" --simulate > /dev/null 2>&1; then
        echo "  ✅ 通过"
        PASSED=$((PASSED + 1))
    else
        echo "  ❌ 失败"
        FAILED=$((FAILED + 1))
    fi
}

echo "1. 基础测试"
run_test "简单常量" "test/test_simple.mlir"
run_test "算术运算" "test/test_arith.mlir"
run_test "复杂计算" "test/test_compute.mlir"
run_test "浮点运算" "test/test_float.mlir"
run_test "比较运算" "test/test_compare.mlir"
run_test "综合测试" "test/test_all.mlir"

echo ""
echo "2. 控制流测试"
run_test "scf.for循环" "test/test_scf_for.mlir"
run_test "scf.if条件" "test/test_scf_if.mlir"
run_test "嵌套控制流" "test/test_nested.mlir"

echo ""
echo "=== 测试结果 ==="
echo "通过: $PASSED"
echo "失败: $FAILED"
echo ""

if [ $FAILED -eq 0 ]; then
    echo "✅ 所有测试通过！"
    exit 0
else
    echo "❌ 有测试失败"
    exit 1
fi
