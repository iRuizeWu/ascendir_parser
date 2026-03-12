#!/bin/bash

# 全面测试脚本 - 运行所有MLIR测试文件

PARSER="./build/src/ascendir_parser"
PASS=0
FAIL=0
FAILED_TESTS=""

echo "=== 全面MLIR仿真器测试 ==="
echo ""

# 获取所有测试文件
TEST_FILES=$(ls test/*.mlir | sort)

for test_file in $TEST_FILES; do
    test_name=$(basename "$test_file" .mlir)
    
    # 运行测试
    output=$($PARSER "$test_file" --simulate 2>&1)
    exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        echo "✅ $test_name"
        ((PASS++))
    else
        echo "❌ $test_name (exit code: $exit_code)"
        ((FAIL++))
        FAILED_TESTS="$FAILED_TESTS\n  - $test_name"
        
        # 显示错误输出的前10行
        echo "  错误输出:"
        echo "$output" | head -10 | sed 's/^/    /'
        echo ""
    fi
done

echo ""
echo "=== 测试结果 ==="
echo "通过: $PASS"
echo "失败: $FAIL"

if [ $FAIL -eq 0 ]; then
    echo ""
    echo "✅ 所有测试通过！"
    exit 0
else
    echo ""
    echo "失败的测试:"
    echo -e "$FAILED_TESTS"
    exit 1
fi
