#include "InstructionRegistry.h"
#include "ExecutionContext.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/Support/raw_ostream.h"

namespace ascendir_parser {

void executeArithConstant(mlir::Operation* op, ExecutionContext& ctx) {
    auto constOp = llvm::cast<mlir::arith::ConstantOp>(op);
    auto value = constOp.getValue();
    
    if (auto intAttr = llvm::dyn_cast<mlir::IntegerAttr>(value)) {
        ctx.setIntValue(constOp.getResult(), intAttr.getValue());
    } else if (auto floatAttr = llvm::dyn_cast<mlir::FloatAttr>(value)) {
        ctx.setFloatValue(constOp.getResult(), floatAttr.getValue());
    }
}

void executeArithAddi(mlir::Operation* op, ExecutionContext& ctx) {
    auto addiOp = llvm::cast<mlir::arith::AddIOp>(op);
    auto lhs = ctx.getIntValue(addiOp.getLhs());
    auto rhs = ctx.getIntValue(addiOp.getRhs());
    ctx.setIntValue(addiOp.getResult(), lhs + rhs);
}

void executeArithSubi(mlir::Operation* op, ExecutionContext& ctx) {
    auto subiOp = llvm::cast<mlir::arith::SubIOp>(op);
    auto lhs = ctx.getIntValue(subiOp.getLhs());
    auto rhs = ctx.getIntValue(subiOp.getRhs());
    ctx.setIntValue(subiOp.getResult(), lhs - rhs);
}

void executeArithMuli(mlir::Operation* op, ExecutionContext& ctx) {
    auto muliOp = llvm::cast<mlir::arith::MulIOp>(op);
    auto lhs = ctx.getIntValue(muliOp.getLhs());
    auto rhs = ctx.getIntValue(muliOp.getRhs());
    ctx.setIntValue(muliOp.getResult(), lhs * rhs);
}

void executeArithDivi(mlir::Operation* op, ExecutionContext& ctx) {
    auto diviOp = llvm::cast<mlir::arith::DivSIOp>(op);
    auto lhs = ctx.getIntValue(diviOp.getLhs());
    auto rhs = ctx.getIntValue(diviOp.getRhs());
    if (rhs != 0) {
        ctx.setIntValue(diviOp.getResult(), lhs.sdiv(rhs));
    }
}

void executeArithAddf(mlir::Operation* op, ExecutionContext& ctx) {
    auto addfOp = llvm::cast<mlir::arith::AddFOp>(op);
    auto lhs = ctx.getFloatValue(addfOp.getLhs());
    auto rhs = ctx.getFloatValue(addfOp.getRhs());
    ctx.setFloatValue(addfOp.getResult(), lhs + rhs);
}

void executeArithSubf(mlir::Operation* op, ExecutionContext& ctx) {
    auto subfOp = llvm::cast<mlir::arith::SubFOp>(op);
    auto lhs = ctx.getFloatValue(subfOp.getLhs());
    auto rhs = ctx.getFloatValue(subfOp.getRhs());
    ctx.setFloatValue(subfOp.getResult(), lhs - rhs);
}

void executeArithMulf(mlir::Operation* op, ExecutionContext& ctx) {
    auto mulfOp = llvm::cast<mlir::arith::MulFOp>(op);
    auto lhs = ctx.getFloatValue(mulfOp.getLhs());
    auto rhs = ctx.getFloatValue(mulfOp.getRhs());
    ctx.setFloatValue(mulfOp.getResult(), lhs * rhs);
}

void executeArithDivf(mlir::Operation* op, ExecutionContext& ctx) {
    auto divfOp = llvm::cast<mlir::arith::DivFOp>(op);
    auto lhs = ctx.getFloatValue(divfOp.getLhs());
    auto rhs = ctx.getFloatValue(divfOp.getRhs());
    ctx.setFloatValue(divfOp.getResult(), lhs / rhs);
}

void executeArithCmpi(mlir::Operation* op, ExecutionContext& ctx) {
    auto cmpiOp = llvm::cast<mlir::arith::CmpIOp>(op);
    auto lhs = ctx.getIntValue(cmpiOp.getLhs());
    auto rhs = ctx.getIntValue(cmpiOp.getRhs());
    auto predicate = cmpiOp.getPredicate();
    
    bool result = false;
    switch (predicate) {
        case mlir::arith::CmpIPredicate::eq:
            result = lhs == rhs;
            break;
        case mlir::arith::CmpIPredicate::ne:
            result = lhs != rhs;
            break;
        case mlir::arith::CmpIPredicate::slt:
            result = lhs.slt(rhs);
            break;
        case mlir::arith::CmpIPredicate::sle:
            result = lhs.sle(rhs);
            break;
        case mlir::arith::CmpIPredicate::sgt:
            result = lhs.sgt(rhs);
            break;
        case mlir::arith::CmpIPredicate::sge:
            result = lhs.sge(rhs);
            break;
        case mlir::arith::CmpIPredicate::ult:
            result = lhs.ult(rhs);
            break;
        case mlir::arith::CmpIPredicate::ule:
            result = lhs.ule(rhs);
            break;
        case mlir::arith::CmpIPredicate::ugt:
            result = lhs.ugt(rhs);
            break;
        case mlir::arith::CmpIPredicate::uge:
            result = lhs.uge(rhs);
            break;
    }
    
    ctx.setIntValue(cmpiOp.getResult(), llvm::APInt(1, result ? 1 : 0));
}

void executeArithIndexCast(mlir::Operation* op, ExecutionContext& ctx) {
    auto indexCastOp = llvm::cast<mlir::arith::IndexCastOp>(op);
    auto input = ctx.getIntValue(indexCastOp.getIn());
    
    auto resultType = indexCastOp.getResult().getType();
    unsigned width = 64;
    if (auto intType = llvm::dyn_cast<mlir::IntegerType>(resultType)) {
        width = intType.getWidth();
    }
    
    llvm::APInt result(width, input.getSExtValue(), true);
    ctx.setIntValue(indexCastOp.getResult(), result);
}

void registerArithInstructions() {
    REGISTER_INSTRUCTION_WITH_LATENCY("arith.constant", executeArithConstant, 1);
    REGISTER_INSTRUCTION_WITH_LATENCY("arith.addi", executeArithAddi, 1);
    REGISTER_INSTRUCTION_WITH_LATENCY("arith.subi", executeArithSubi, 1);
    REGISTER_INSTRUCTION_WITH_LATENCY("arith.muli", executeArithMuli, 3);
    REGISTER_INSTRUCTION_WITH_LATENCY("arith.divsi", executeArithDivi, 10);
    REGISTER_INSTRUCTION_WITH_LATENCY("arith.addf", executeArithAddf, 2);
    REGISTER_INSTRUCTION_WITH_LATENCY("arith.subf", executeArithSubf, 2);
    REGISTER_INSTRUCTION_WITH_LATENCY("arith.mulf", executeArithMulf, 4);
    REGISTER_INSTRUCTION_WITH_LATENCY("arith.divf", executeArithDivf, 12);
    REGISTER_INSTRUCTION_WITH_LATENCY("arith.cmpi", executeArithCmpi, 1);
    REGISTER_INSTRUCTION_WITH_LATENCY("arith.index_cast", executeArithIndexCast, 5);
}

}
