#include <gtest/gtest.h>
#include "IsaSequence.h"
#include "Isa.h"
#include "Instructions/ControlFlowIsas.h"

using namespace ascendir_parser;

class IsaSequenceTest : public ::testing::Test {
protected:
    IsaSequence sequence;

    IsaPtr createTestIsa(const std::string& name) {
        auto isa = std::make_unique<JumpIsa>();
        isa->setOpName(name);
        return isa;
    }
};

TEST_F(IsaSequenceTest, AddInstruction) {
    auto isa = createTestIsa("test_op");
    sequence.addInstruction(std::move(isa));

    EXPECT_EQ(sequence.size(), 1);
}

TEST_F(IsaSequenceTest, AddMultipleInstructions) {
    for (int i = 0; i < 5; ++i) {
        auto isa = createTestIsa("op_" + std::to_string(i));
        sequence.addInstruction(std::move(isa));
    }

    EXPECT_EQ(sequence.size(), 5);
}

TEST_F(IsaSequenceTest, AllocatePC) {
    uint64_t pc1 = sequence.allocatePC();
    EXPECT_EQ(pc1, 0);

    auto isa = createTestIsa("test");
    sequence.addInstruction(std::move(isa));

    uint64_t pc2 = sequence.allocatePC();
    EXPECT_EQ(pc2, 1);
    EXPECT_LT(pc1, pc2);
}

TEST_F(IsaSequenceTest, LabelManagement) {
    uint64_t pc = 0x100;
    sequence.setLabel("loop_start", pc);

    uint64_t retrievedPC = sequence.getLabelPC("loop_start");
    EXPECT_EQ(retrievedPC, pc);
}

TEST_F(IsaSequenceTest, GetInstructionByPC) {
    auto isa1 = createTestIsa("op1");
    auto isa2 = createTestIsa("op2");

    sequence.addInstruction(std::move(isa1));
    sequence.addInstruction(std::move(isa2));

    Isa* retrieved = sequence.getInstructionByPC(0);
    EXPECT_NE(retrieved, nullptr);
}

TEST_F(IsaSequenceTest, IsValidPC) {
    auto isa = createTestIsa("test_op");
    sequence.addInstruction(std::move(isa));

    EXPECT_TRUE(sequence.isValidPC(0));
    EXPECT_FALSE(sequence.isValidPC(999));
}

TEST_F(IsaSequenceTest, UpdateJumpTarget) {
    auto jump = createTestIsa("jump");
    sequence.addInstruction(std::move(jump));

    uint64_t targetPC = 0x200;
    sequence.updateJumpTarget(0, targetPC);

    EXPECT_TRUE(sequence.isValidPC(0));
}

TEST_F(IsaSequenceTest, UpdateConditionalJump) {
    auto cond_jump = createTestIsa("cond_jump");
    sequence.addInstruction(std::move(cond_jump));

    sequence.updateConditionalJump(0, 0x100, 0x200);

    EXPECT_TRUE(sequence.isValidPC(0));
}

TEST_F(IsaSequenceTest, GetNextPC) {
    auto isa1 = createTestIsa("op1");
    auto isa2 = createTestIsa("op2");

    sequence.addInstruction(std::move(isa1));
    sequence.addInstruction(std::move(isa2));

    uint64_t nextPC = sequence.getNextPC(0);
    EXPECT_GT(nextPC, 0);
}

TEST_F(IsaSequenceTest, SequenceSize) {
    EXPECT_EQ(sequence.size(), 0);

    for (int i = 0; i < 10; ++i) {
        sequence.addInstruction(createTestIsa("op"));
    }

    EXPECT_EQ(sequence.size(), 10);
}
