#pragma once

#include "Isa.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Operation.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APFloat.h"
#include <algorithm>

namespace ascendir_parser {

class ArithConstantIsa : public Isa {
public:
    ArithConstantIsa() {
        isaName = IsaName::ArithConstant;
    }
    
    ExecutionUnitType getExecutionUnit() const override {
        return ExecutionUnitType::Scalar;
    }
    
    uint64_t getLatency() const override { return 1; }
    
    void execute(ExecutionContext& ctx) override {
        auto constOp = llvm::cast<mlir::arith::ConstantOp>(mlirOp);
        auto value = constOp.getValue();
        
        if (auto intAttr = llvm::dyn_cast<mlir::IntegerAttr>(value)) {
            ctx.setIntValue(constOp.getResult(), intAttr.getValue());
        } else if (auto floatAttr = llvm::dyn_cast<mlir::FloatAttr>(value)) {
            ctx.setFloatValue(constOp.getResult(), floatAttr.getValue());
        }
        // IndexAttr is handled as IntegerAttr in MLIR
    }
    
    std::string getDescription() const override {
        return "arith.constant";
    }
};

class ArithAddiIsa : public Isa {
public:
    uint64_t getLatency() const override { return 1; }
    
    void execute(ExecutionContext& ctx) override {
        auto addiOp = llvm::cast<mlir::arith::AddIOp>(mlirOp);
        auto lhs = ctx.getIntValue(addiOp.getLhs());
        auto rhs = ctx.getIntValue(addiOp.getRhs());
        
        // 确保位宽一致
        unsigned maxBits = std::max(lhs.getBitWidth(), rhs.getBitWidth());
        llvm::APInt lhsExt = (lhs.getBitWidth() < maxBits) ? lhs.sext(maxBits) : lhs;
        llvm::APInt rhsExt = (rhs.getBitWidth() < maxBits) ? rhs.sext(maxBits) : rhs;
        
        ctx.setIntValue(addiOp.getResult(), lhsExt + rhsExt);
    }
    
    std::string getDescription() const override {
        return "arith.addi";
    }
};

class ArithSubiIsa : public Isa {
public:
    uint64_t getLatency() const override { return 1; }
    
    void execute(ExecutionContext& ctx) override {
        auto subiOp = llvm::cast<mlir::arith::SubIOp>(mlirOp);
        auto lhs = ctx.getIntValue(subiOp.getLhs());
        auto rhs = ctx.getIntValue(subiOp.getRhs());
        
        // 确保位宽一致
        unsigned maxBits = std::max(lhs.getBitWidth(), rhs.getBitWidth());
        llvm::APInt lhsExt = (lhs.getBitWidth() < maxBits) ? lhs.sext(maxBits) : lhs;
        llvm::APInt rhsExt = (rhs.getBitWidth() < maxBits) ? rhs.sext(maxBits) : rhs;
        
        ctx.setIntValue(subiOp.getResult(), lhsExt - rhsExt);
    }
    
    std::string getDescription() const override {
        return "arith.subi";
    }
};

class ArithMuliIsa : public Isa {
public:
    uint64_t getLatency() const override { return 3; }
    
    void execute(ExecutionContext& ctx) override {
        auto muliOp = llvm::cast<mlir::arith::MulIOp>(mlirOp);
        auto lhs = ctx.getIntValue(muliOp.getLhs());
        auto rhs = ctx.getIntValue(muliOp.getRhs());
        
        // 确保位宽一致
        unsigned maxBits = std::max(lhs.getBitWidth(), rhs.getBitWidth());
        llvm::APInt lhsExt = (lhs.getBitWidth() < maxBits) ? lhs.sext(maxBits) : lhs;
        llvm::APInt rhsExt = (rhs.getBitWidth() < maxBits) ? rhs.sext(maxBits) : rhs;
        
        ctx.setIntValue(muliOp.getResult(), lhsExt * rhsExt);
    }
    
    std::string getDescription() const override {
        return "arith.muli";
    }
};

class ArithDiviIsa : public Isa {
public:
    uint64_t getLatency() const override { return 10; }
    
    void execute(ExecutionContext& ctx) override {
        auto diviOp = llvm::cast<mlir::arith::DivSIOp>(mlirOp);
        auto lhs = ctx.getIntValue(diviOp.getLhs());
        auto rhs = ctx.getIntValue(diviOp.getRhs());
        
        auto resultType = diviOp.getResult().getType();
        unsigned width = 64;
        if (auto intType = llvm::dyn_cast<mlir::IntegerType>(resultType)) {
            width = intType.getWidth();
        }
        
        unsigned maxBits = std::max({width, lhs.getBitWidth(), rhs.getBitWidth()});
        llvm::APInt lhsExt = (lhs.getBitWidth() < maxBits) ? lhs.sext(maxBits) : lhs;
        llvm::APInt rhsExt = (rhs.getBitWidth() < maxBits) ? rhs.sext(maxBits) : rhs;
        
        if (rhsExt != 0) {
            llvm::APInt result = lhsExt.sdiv(rhsExt);
            if (result.getBitWidth() != width) {
                result = result.trunc(width);
            }
            ctx.setIntValue(diviOp.getResult(), result);
        }
    }
    
    std::string getDescription() const override {
        return "arith.divsi";
    }
};

class ArithAddfIsa : public Isa {
public:
    uint64_t getLatency() const override { return 2; }
    
    void execute(ExecutionContext& ctx) override {
        auto addfOp = llvm::cast<mlir::arith::AddFOp>(mlirOp);
        auto lhs = ctx.getFloatValue(addfOp.getLhs());
        auto rhs = ctx.getFloatValue(addfOp.getRhs());
        ctx.setFloatValue(addfOp.getResult(), lhs + rhs);
    }
    
    std::string getDescription() const override {
        return "arith.addf";
    }
};

class ArithSubfIsa : public Isa {
public:
    uint64_t getLatency() const override { return 2; }
    
    void execute(ExecutionContext& ctx) override {
        auto subfOp = llvm::cast<mlir::arith::SubFOp>(mlirOp);
        auto lhs = ctx.getFloatValue(subfOp.getLhs());
        auto rhs = ctx.getFloatValue(subfOp.getRhs());
        ctx.setFloatValue(subfOp.getResult(), lhs - rhs);
    }
    
    std::string getDescription() const override {
        return "arith.subf";
    }
};

class ArithMulfIsa : public Isa {
public:
    uint64_t getLatency() const override { return 4; }
    
    void execute(ExecutionContext& ctx) override {
        auto mulfOp = llvm::cast<mlir::arith::MulFOp>(mlirOp);
        auto lhs = ctx.getFloatValue(mulfOp.getLhs());
        auto rhs = ctx.getFloatValue(mulfOp.getRhs());
        ctx.setFloatValue(mulfOp.getResult(), lhs * rhs);
    }
    
    std::string getDescription() const override {
        return "arith.mulf";
    }
};

class ArithDivfIsa : public Isa {
public:
    uint64_t getLatency() const override { return 12; }
    
    void execute(ExecutionContext& ctx) override {
        auto divfOp = llvm::cast<mlir::arith::DivFOp>(mlirOp);
        auto lhs = ctx.getFloatValue(divfOp.getLhs());
        auto rhs = ctx.getFloatValue(divfOp.getRhs());
        ctx.setFloatValue(divfOp.getResult(), lhs / rhs);
    }
    
    std::string getDescription() const override {
        return "arith.divf";
    }
};

class ArithCmpiIsa : public Isa {
public:
    uint64_t getLatency() const override { return 1; }
    
    void execute(ExecutionContext& ctx) override {
        auto cmpiOp = llvm::cast<mlir::arith::CmpIOp>(mlirOp);
        auto lhs = ctx.getIntValue(cmpiOp.getLhs());
        auto rhs = ctx.getIntValue(cmpiOp.getRhs());
        auto predicate = cmpiOp.getPredicate();
        
        unsigned maxBits = std::max(lhs.getBitWidth(), rhs.getBitWidth());
        llvm::APInt lhsExt = (lhs.getBitWidth() < maxBits) ? lhs.sext(maxBits) : lhs;
        llvm::APInt rhsExt = (rhs.getBitWidth() < maxBits) ? rhs.sext(maxBits) : rhs;
        
        bool result = false;
        switch (predicate) {
            case mlir::arith::CmpIPredicate::eq:
                result = lhsExt == rhsExt;
                break;
            case mlir::arith::CmpIPredicate::ne:
                result = lhsExt != rhsExt;
                break;
            case mlir::arith::CmpIPredicate::slt:
                result = lhsExt.slt(rhsExt);
                break;
            case mlir::arith::CmpIPredicate::sle:
                result = lhsExt.sle(rhsExt);
                break;
            case mlir::arith::CmpIPredicate::sgt:
                result = lhsExt.sgt(rhsExt);
                break;
            case mlir::arith::CmpIPredicate::sge:
                result = lhsExt.sge(rhsExt);
                break;
            case mlir::arith::CmpIPredicate::ult:
                result = lhsExt.ult(rhsExt);
                break;
            case mlir::arith::CmpIPredicate::ule:
                result = lhsExt.ule(rhsExt);
                break;
            case mlir::arith::CmpIPredicate::ugt:
                result = lhsExt.ugt(rhsExt);
                break;
            case mlir::arith::CmpIPredicate::uge:
                result = lhsExt.uge(rhsExt);
                break;
        }
        
        ctx.setIntValue(cmpiOp.getResult(), llvm::APInt(1, result ? 1 : 0));
    }
    
    std::string getDescription() const override {
        return "arith.cmpi";
    }
};

class ArithIndexCastIsa : public Isa {
public:
    uint64_t getLatency() const override { return 5; }
    
    void execute(ExecutionContext& ctx) override {
        auto indexCastOp = llvm::cast<mlir::arith::IndexCastOp>(mlirOp);
        auto input = ctx.getIntValue(indexCastOp.getIn());
        
        auto resultType = indexCastOp.getResult().getType();
        unsigned width = 64;
        if (auto intType = llvm::dyn_cast<mlir::IntegerType>(resultType)) {
            width = intType.getWidth();
        }
        
        llvm::APInt result(width, input.getSExtValue(), true);
        ctx.setIntValue(indexCastOp.getResult(), result);
    }
    
    std::string getDescription() const override {
        return "arith.index_cast";
    }
};

inline void registerArithIsas() {
    REGISTER_ISA("arith.constant", ArithConstantIsa, IsaName::ArithConstant);
    REGISTER_ISA("arith.addi", ArithAddiIsa, IsaName::ArithAddi);
    REGISTER_ISA("arith.subi", ArithSubiIsa, IsaName::ArithSubi);
    REGISTER_ISA("arith.muli", ArithMuliIsa, IsaName::ArithMuli);
    REGISTER_ISA("arith.divsi", ArithDiviIsa, IsaName::ArithDivi);
    REGISTER_ISA("arith.addf", ArithAddfIsa, IsaName::ArithAddf);
    REGISTER_ISA("arith.subf", ArithSubfIsa, IsaName::ArithSubf);
    REGISTER_ISA("arith.mulf", ArithMulfIsa, IsaName::ArithMulf);
    REGISTER_ISA("arith.divf", ArithDivfIsa, IsaName::ArithDivf);
    REGISTER_ISA("arith.cmpi", ArithCmpiIsa, IsaName::ArithCmpi);
    REGISTER_ISA("arith.index_cast", ArithIndexCastIsa, IsaName::ArithIndexCast);
}

}