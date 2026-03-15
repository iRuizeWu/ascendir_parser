#!/bin/bash
# Build and run tests for ascendir_parser

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}Building ascendir_parser tests...${NC}"

# Check if LLVM_DIR is set
if [ -z "$LLVM_DIR" ]; then
    echo -e "${RED}Error: LLVM_DIR environment variable not set${NC}"
    echo "Please set LLVM_DIR before running tests"
    echo "Example: export LLVM_DIR=/path/to/llvm/install"
    exit 1
fi

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Configure CMake with test support
cmake -DCMAKE_BUILD_TYPE=Debug \
       -DLLVM_DIR="$LLVM_DIR" \
       -DMLIR_DIR="${MLIR_DIR:-$LLVM_DIR/lib/cmake/mlir}" \
       ..

# Build tests
make ascendir_tests -j$(nproc)

echo -e "${GREEN}Build successful!${NC}"
echo -e "${YELLOW}Running tests...${NC}"

# Run tests
ctest --output-on-failure

echo -e "${GREEN}All tests completed!${NC}"
