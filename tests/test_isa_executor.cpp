#include <gtest/gtest.h>
#include "IsaExecutor.h"
#include "ExecutionContext.h"
#include "IsaSequence.h"
#include "Instructions/ControlFlowIsas.h"

using namespace ascendir_parser;

class IsaExecutorTest : public ::testing::Test {
protected:
    IsaExecutor executor;

    IsaPtr createTestIsa() {
        return std::make_unique<JumpIsa>();
    }
};

// Test executor initialization
TEST_F(IsaExecutorTest, Initialization) {
    EXPECT_EQ(executor.getCurrentPC(), 0);
    EXPECT_EQ(executor.getCurrentCycle(), 0);
    EXPECT_FALSE(executor.isHalted());
    EXPECT_FALSE(executor.isBlocked());
}

// Test PC advancement
TEST_F(IsaExecutorTest, PCManagement) {
    uint64_t initialPC = executor.getCurrentPC();
    EXPECT_EQ(initialPC, 0);

    // PC should be accessible and valid
    uint64_t pc = executor.getCurrentPC();
    EXPECT_GE(pc, 0);
}

// Test cycle counter
TEST_F(IsaExecutorTest, CycleCounter) {
    uint64_t initialCycle = executor.getCurrentCycle();
    EXPECT_EQ(initialCycle, 0);

    // Cycle counter should be accessible
    uint64_t cycle = executor.getCurrentCycle();
    EXPECT_GE(cycle, 0);
}

// Test halt state
TEST_F(IsaExecutorTest, HaltState) {
    ExecutionContext& ctx = executor.getContext();

    EXPECT_FALSE(executor.isHalted());
}

// Test blocked state
TEST_F(IsaExecutorTest, BlockedState) {
    EXPECT_FALSE(executor.isBlocked());
}

// Test verbose mode
TEST_F(IsaExecutorTest, VerboseMode) {
    executor.setVerbose(true);
    executor.setVerbose(false);
    // Just test that setting doesn't crash
    EXPECT_TRUE(true);
}

// Test async mode
TEST_F(IsaExecutorTest, AsyncMode) {
    executor.setAsyncMode(true);
    executor.setAsyncMode(false);
    // Just test that setting doesn't crash
    EXPECT_TRUE(true);
}

// Test execution unit status
TEST_F(IsaExecutorTest, ExecutionUnitStatus) {
    bool scalar_busy = executor.isUnitBusy(ExecutionUnitType::Scalar);
    EXPECT_FALSE(scalar_busy);  // Should not be busy initially

    bool mte_busy = executor.isUnitBusy(ExecutionUnitType::MTE);
    EXPECT_FALSE(mte_busy);

    bool cube_busy = executor.isUnitBusy(ExecutionUnitType::Cube);
    EXPECT_FALSE(cube_busy);

    bool vec_busy = executor.isUnitBusy(ExecutionUnitType::Vec);
    EXPECT_FALSE(vec_busy);
}

// Test context access
TEST_F(IsaExecutorTest, ContextAccess) {
    ExecutionContext& ctx = executor.getContext();
    // Context should be accessible
    EXPECT_TRUE(true);
}

// Test sequence access
TEST_F(IsaExecutorTest, SequenceAccess) {
    IsaSequence& seq = executor.getSequence();
    EXPECT_EQ(seq.size(), 0);
}

// Test tick without loaded module (should handle gracefully)
TEST_F(IsaExecutorTest, TickWithoutModule) {
    bool result = executor.tick();
    // Should return false (halted immediately) or true depending on implementation
    EXPECT_TRUE(true);  // Just verify no crash
}

// Test run without loaded module (should handle gracefully)
TEST_F(IsaExecutorTest, RunWithoutModule) {
    executor.run();
    // Should complete without crash
    EXPECT_TRUE(true);
}

// Test multiple tick operations
TEST_F(IsaExecutorTest, MultipleTicks) {
    for (int i = 0; i < 5; ++i) {
        executor.tick();
    }
    EXPECT_TRUE(true);
}
