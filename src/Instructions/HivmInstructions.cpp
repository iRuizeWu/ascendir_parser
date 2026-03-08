#include "InstructionRegistry.h"
#include "ExecutionContext.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/BuiltinTypes.h"
#include "llvm/Support/raw_ostream.h"
#include <cstring>
#include <cmath>

namespace ascendir_parser {

namespace {

size_t getMemrefSize(mlir::Value memref) {
    auto type = memref.getType();
    if (auto memrefType = llvm::dyn_cast<mlir::MemRefType>(type)) {
        auto shape = memrefType.getShape();
        auto elementType = memrefType.getElementType();
        
        size_t numElements = 1;
        for (int64_t dim : shape) {
            if (dim > 0) {
                numElements *= static_cast<size_t>(dim);
            }
        }
        
        size_t elementSize = 0;
        if (auto intType = llvm::dyn_cast<mlir::IntegerType>(elementType)) {
            elementSize = intType.getWidth() / 8;
        } else if (auto floatType = llvm::dyn_cast<mlir::FloatType>(elementType)) {
            elementSize = floatType.getWidth() / 8;
        }
        
        return numElements * elementSize;
    }
    return 0;
}

size_t getMemrefNumElements(mlir::Value memref) {
    auto type = memref.getType();
    if (auto memrefType = llvm::dyn_cast<mlir::MemRefType>(type)) {
        auto shape = memrefType.getShape();
        size_t numElements = 1;
        for (int64_t dim : shape) {
            if (dim > 0) {
                numElements *= static_cast<size_t>(dim);
            }
        }
        return numElements;
    }
    return 0;
}

mlir::Type getElementType(mlir::Value memref) {
    auto type = memref.getType();
    if (auto memrefType = llvm::dyn_cast<mlir::MemRefType>(type)) {
        return memrefType.getElementType();
    }
    return mlir::Type();
}

template<typename T>
T convertAPFloatToType(const llvm::APFloat& apf, const llvm::fltSemantics& semantics);

template<>
llvm::APFloat convertAPFloatToType<llvm::APFloat>(const llvm::APFloat& apf, const llvm::fltSemantics& semantics) {
    return apf;
}

template<>
float convertAPFloatToType<float>(const llvm::APFloat& apf, const llvm::fltSemantics& semantics) {
    llvm::APFloat converted = apf;
    bool ignored;
    converted.convert(llvm::APFloat::IEEEsingle(), llvm::APFloat::rmNearestTiesToEven, &ignored);
    return converted.convertToFloat();
}

template<>
uint16_t convertAPFloatToType<uint16_t>(const llvm::APFloat& apf, const llvm::fltSemantics& semantics) {
    llvm::APFloat converted = apf;
    bool ignored;
    converted.convert(llvm::APFloat::IEEEhalf(), llvm::APFloat::rmNearestTiesToEven, &ignored);
    return static_cast<uint16_t>(converted.bitcastToAPInt().getZExtValue());
}

llvm::APFloat createAPFloatFromBits(mlir::Type elementType, uint16_t bits) {
    if (elementType.isF16()) {
        return llvm::APFloat(llvm::APFloat::IEEEhalf(), llvm::APInt(16, bits));
    } else if (elementType.isF32()) {
        uint32_t f32Bits = static_cast<uint32_t>(bits);
        return llvm::APFloat(llvm::APFloat::IEEEsingle(), llvm::APInt(32, f32Bits));
    } else if (elementType.isBF16()) {
        return llvm::APFloat(llvm::APFloat::BFloat(), llvm::APInt(16, bits));
    }
    return llvm::APFloat(0.0f);
}

}

void executeMemrefAlloc(mlir::Operation* op, ExecutionContext& ctx) {
    auto allocOp = llvm::cast<mlir::memref::AllocOp>(op);
    auto result = allocOp.getResult();
    
    size_t size = getMemrefSize(result);
    std::vector<uint8_t> buffer(size, 0);
    ctx.setMemrefData(result, std::move(buffer));
    
    if (auto memrefType = llvm::dyn_cast<mlir::MemRefType>(result.getType())) {
        llvm::outs() << "  [memref.alloc] Allocated " << size << " bytes for memref<";
        auto shape = memrefType.getShape();
        for (size_t i = 0; i < shape.size(); ++i) {
            if (i > 0) llvm::outs() << "x";
            llvm::outs() << shape[i];
        }
        llvm::outs() << "x" << memrefType.getElementType() << ">\n";
    }
}

void executeHivmHirLoad(mlir::Operation* op, ExecutionContext& ctx) {
    auto operands = op->getOperands();
    
    if (operands.size() < 2) {
        llvm::outs() << "  [hivm.hir.load] Error: insufficient operands\n";
        return;
    }
    
    mlir::Value src = operands[0];
    mlir::Value dst = operands[1];
    
    llvm::outs() << "  [hivm.hir.load] GM -> UB\n";
    
    if (ctx.hasMemrefData(src)) {
        auto& srcData = ctx.getMemrefData(src);
        ctx.setMemrefData(dst, srcData);
        llvm::outs() << "    Copied " << srcData.size() << " bytes\n";
    } else {
        size_t size = getMemrefSize(dst);
        size_t numElements = getMemrefNumElements(dst);
        auto elementType = getElementType(dst);
        std::vector<uint8_t> buffer(size, 0);
        
        llvm::outs() << "    Initialized " << numElements << " elements (test data): [";
        if (elementType.isF16()) {
            uint16_t* ptr = reinterpret_cast<uint16_t*>(buffer.data());
            for (size_t i = 0; i < numElements; ++i) {
                float val = static_cast<float>(i);
                llvm::APFloat apf(val);
                bool ignored;
                apf.convert(llvm::APFloat::IEEEhalf(), llvm::APFloat::rmNearestTiesToEven, &ignored);
                ptr[i] = static_cast<uint16_t>(apf.bitcastToAPInt().getZExtValue());
                
                if (i < 4 || i >= numElements - 2) {
                    if (i > 0 && i < numElements) llvm::outs() << ", ";
                    llvm::outs() << val;
                } else if (i == 4) {
                    llvm::outs() << ", ...";
                }
            }
        }
        llvm::outs() << "]\n";
        
        ctx.setMemrefData(dst, std::move(buffer));
    }
}

void executeHivmHirVadd(mlir::Operation* op, ExecutionContext& ctx) {
    auto operands = op->getOperands();
    
    if (operands.size() < 3) {
        llvm::outs() << "  [hivm.hir.vadd] Error: insufficient operands\n";
        return;
    }
    
    mlir::Value srcA = operands[0];
    mlir::Value srcB = operands[1];
    mlir::Value dst = operands[2];
    
    llvm::outs() << "  [hivm.hir.vadd] Vector addition\n";
    
    if (ctx.hasMemrefData(srcA) && ctx.hasMemrefData(srcB)) {
        auto& dataA = ctx.getMemrefData(srcA);
        auto& dataB = ctx.getMemrefData(srcB);
        
        size_t numElements = getMemrefNumElements(dst);
        auto elementType = getElementType(dst);
        
        std::vector<uint8_t> resultData(dataA.size());
        
        llvm::outs() << "    Result [" << numElements << " elements]: [";
        if (elementType.isF16()) {
            uint16_t* ptrA = reinterpret_cast<uint16_t*>(dataA.data());
            uint16_t* ptrB = reinterpret_cast<uint16_t*>(dataB.data());
            uint16_t* ptrR = reinterpret_cast<uint16_t*>(resultData.data());
            
            for (size_t i = 0; i < numElements; ++i) {
                llvm::APFloat a(llvm::APFloat::IEEEhalf(), llvm::APInt(16, ptrA[i]));
                llvm::APFloat b(llvm::APFloat::IEEEhalf(), llvm::APInt(16, ptrB[i]));
                a.add(b, llvm::APFloat::rmNearestTiesToEven);
                ptrR[i] = static_cast<uint16_t>(a.bitcastToAPInt().getZExtValue());
                
                if (i < 4 || i >= numElements - 2) {
                    if (i > 0 && i < numElements) llvm::outs() << ", ";
                    llvm::SmallString<16> str;
                    a.toString(str);
                    llvm::outs() << str;
                } else if (i == 4) {
                    llvm::outs() << ", ...";
                }
            }
        }
        llvm::outs() << "]\n";
        
        ctx.setMemrefData(dst, std::move(resultData));
    }
}

void executeHivmHirStore(mlir::Operation* op, ExecutionContext& ctx) {
    auto operands = op->getOperands();
    
    if (operands.size() < 2) {
        llvm::outs() << "  [hivm.hir.store] Error: insufficient operands\n";
        return;
    }
    
    mlir::Value src = operands[0];
    mlir::Value dst = operands[1];
    
    llvm::outs() << "  [hivm.hir.store] UB -> GM\n";
    
    if (ctx.hasMemrefData(src)) {
        auto& srcData = ctx.getMemrefData(src);
        size_t numElements = getMemrefNumElements(dst);
        auto elementType = getElementType(dst);
        
        llvm::outs() << "    Stored " << srcData.size() << " bytes [" << numElements << " elements]: [";
        if (elementType.isF16()) {
            uint16_t* ptr = reinterpret_cast<uint16_t*>(srcData.data());
            for (size_t i = 0; i < numElements; ++i) {
                if (i < 4 || i >= numElements - 2) {
                    if (i > 0 && i < numElements) llvm::outs() << ", ";
                    llvm::APFloat val(llvm::APFloat::IEEEhalf(), llvm::APInt(16, ptr[i]));
                    llvm::SmallString<16> str;
                    val.toString(str);
                    llvm::outs() << str;
                } else if (i == 4) {
                    llvm::outs() << ", ...";
                }
            }
        }
        llvm::outs() << "]\n";
        
        ctx.setMemrefData(dst, srcData);
    }
}

void registerHivmInstructions() {
    REGISTER_INSTRUCTION_WITH_LATENCY("memref.alloc", executeMemrefAlloc, 1);
    REGISTER_INSTRUCTION_WITH_LATENCY("hivm.hir.load", executeHivmHirLoad, 100);
    REGISTER_INSTRUCTION_WITH_LATENCY("hivm.hir.vadd", executeHivmHirVadd, 50);
    REGISTER_INSTRUCTION_WITH_LATENCY("hivm.hir.store", executeHivmHirStore, 100);
}

}
