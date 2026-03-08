#include "InstructionRegistry.h"
#include "ExecutionContext.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "llvm/Support/raw_ostream.h"

namespace ascendir_parser {

void executeFuncCall(mlir::Operation* op, ExecutionContext& ctx) {
    llvm::errs() << "func.call not yet implemented\n";
    ctx.halt();
}

void executeFuncReturn(mlir::Operation* op, ExecutionContext& ctx) {
    auto returnOp = llvm::cast<mlir::func::ReturnOp>(op);
    
    if (!ctx.hasCallFrames()) {
        ctx.halt();
    } else {
        uint64_t returnPC = ctx.popCallFrame();
        ctx.setPC(returnPC);
    }
}

void registerFuncInstructions() {
    REGISTER_INSTRUCTION_WITH_LATENCY("func.call", executeFuncCall, 5);
    REGISTER_INSTRUCTION_WITH_LATENCY("func.return", executeFuncReturn, 1);
}

}
