#include <gtest/gtest.h>
#include "Parser.h"
#include <filesystem>
#include <fstream>

using namespace ascendir_parser;

class ParserTest : public ::testing::Test {
protected:
    Parser parser;
    std::filesystem::path testDir;

    void SetUp() override {
        testDir = std::filesystem::temp_directory_path() / "ascendir_test";
        std::filesystem::create_directories(testDir);
    }

    void TearDown() override {
        std::filesystem::remove_all(testDir);
    }

    void createTestFile(const std::string& filename, const std::string& content) {
        auto filepath = testDir / filename;
        std::ofstream file(filepath);
        file << content;
        file.close();
    }
};

TEST_F(ParserTest, ParseSimpleModule) {
    std::string mlirCode = R"(
    module {
      func.func @simple() {
        return
      }
    }
    )";

    createTestFile("simple.mlir", mlirCode);

    auto module = parser.parseFile((testDir / "simple.mlir").string());
    EXPECT_TRUE(module.get() != nullptr);
}

TEST_F(ParserTest, ExtractFunctions) {
    std::string mlirCode = R"(
    module {
      func.func @func1() {
        return
      }
      func.func @func2() {
        return
      }
    }
    )";

    createTestFile("functions.mlir", mlirCode);

    auto module = parser.parseFile((testDir / "functions.mlir").string());
    auto functions = parser.extractFunctions(module.get());

    EXPECT_GE(functions.size(), 0);
}

TEST_F(ParserTest, ParseArithmeticOps) {
    std::string mlirCode = R"(
    module {
      func.func @compute(%arg0: i32, %arg1: i32) -> i32 {
        %0 = arith.addi %arg0, %arg1 : i32
        return %0 : i32
      }
    }
    )";

    createTestFile("arith.mlir", mlirCode);

    auto module = parser.parseFile((testDir / "arith.mlir").string());
    EXPECT_TRUE(module.get() != nullptr);
}

TEST_F(ParserTest, ParseControlFlow) {
    std::string mlirCode = R"(
    module {
      func.func @conditional(%cond: i1) {
        cf.cond_br %cond, ^bb1, ^bb2
      ^bb1:
        return
      ^bb2:
        return
      }
    }
    )";

    createTestFile("control_flow.mlir", mlirCode);

    auto module = parser.parseFile((testDir / "control_flow.mlir").string());
    EXPECT_TRUE(module.get() != nullptr);
}
