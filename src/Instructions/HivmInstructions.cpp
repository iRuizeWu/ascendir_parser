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

size_t calculateLoadStoreDataSize(mlir::Operation* op, ExecutionContext& ctx) {
    auto operands = op->getOperands();
    if (operands.size() >= 2) {
        return getMemrefSize(operands[0]);
    }
    return 0;
}

size_t calculateVaddDataSize(mlir::Operation* op, ExecutionContext& ctx) {
    auto operands = op->getOperands();
    if (operands.size() >= 3) {
        return getMemrefSize(operands[0]);
    }
    return 0;
}

size_t calculateMatmulDataSize(mlir::Operation* op, ExecutionContext& ctx) {
    auto operands = op->getOperands();
    if (operands.size() >= 3) {
        mlir::Value a = operands[0];
        mlir::Value b = operands[1];
        return getMemrefSize(a) + getMemrefSize(b);
    }
    return 0;
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
    
    size_t dataSize = getMemrefSize(dst);
    size_t numElements = getMemrefNumElements(dst);
    auto elementType = getElementType(dst);
    
    llvm::outs() << "  [hivm.hir.load] GM -> UB on MTE\n";
    llvm::outs() << "    Data size: " << dataSize << " bytes (" << numElements << " elements)\n";
    
    if (ctx.hasMemrefData(src)) {
        auto& srcData = ctx.getMemrefData(src);
        ctx.setMemrefData(dst, srcData);
        llvm::outs() << "    Copied " << srcData.size() << " bytes\n";
    } else {
        std::vector<uint8_t> buffer(dataSize, 0);
        
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
    
    size_t dataSize = getMemrefSize(dst);
    size_t numElements = getMemrefNumElements(dst);
    auto elementType = getElementType(dst);
    
    llvm::outs() << "  [hivm.hir.vadd] Vector addition on VEC\n";
    llvm::outs() << "    Data size: " << dataSize << " bytes (" << numElements << " elements)\n";
    
    if (ctx.hasMemrefData(srcA) && ctx.hasMemrefData(srcB)) {
        auto& dataA = ctx.getMemrefData(srcA);
        auto& dataB = ctx.getMemrefData(srcB);
        
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
    
    size_t dataSize = getMemrefSize(src);
    size_t numElements = getMemrefNumElements(dst);
    auto elementType = getElementType(dst);
    
    llvm::outs() << "  [hivm.hir.store] UB -> GM on MTE\n";
    llvm::outs() << "    Data size: " << dataSize << " bytes (" << numElements << " elements)\n";
    
    if (ctx.hasMemrefData(src)) {
        auto& srcData = ctx.getMemrefData(src);
        
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

void executeHivmHirMatmul(mlir::Operation* op, ExecutionContext& ctx) {
    auto operands = op->getOperands();
    
    if (operands.size() < 3) {
        llvm::outs() << "  [hivm.hir.matmul] Error: insufficient operands\n";
        return;
    }
    
    mlir::Value srcA = operands[0];
    mlir::Value srcB = operands[1];
    mlir::Value dst = operands[2];
    
    auto typeA = srcA.getType();
    auto typeB = srcB.getType();
    auto typeDst = dst.getType();
    
    auto memrefA = llvm::dyn_cast<mlir::MemRefType>(typeA);
    auto memrefB = llvm::dyn_cast<mlir::MemRefType>(typeB);
    auto memrefDst = llvm::dyn_cast<mlir::MemRefType>(typeDst);
    
    size_t dataSizeA = getMemrefSize(srcA);
    size_t dataSizeB = getMemrefSize(srcB);
    size_t totalDataSize = dataSizeA + dataSizeB;
    
    llvm::outs() << "  [hivm.hir.matmul] Matrix multiplication on CUBE\n";
    llvm::outs() << "    Matrix A: " << dataSizeA << " bytes";
    if (memrefA) {
        auto shape = memrefA.getShape();
        llvm::outs() << " (";
        for (size_t i = 0; i < shape.size(); ++i) {
            if (i > 0) llvm::outs() << "x";
            llvm::outs() << shape[i];
        }
        llvm::outs() << ")";
    }
    llvm::outs() << "\n";
    
    llvm::outs() << "    Matrix B: " << dataSizeB << " bytes";
    if (memrefB) {
        auto shape = memrefB.getShape();
        llvm::outs() << " (";
        for (size_t i = 0; i < shape.size(); ++i) {
            if (i > 0) llvm::outs() << "x";
            llvm::outs() << shape[i];
        }
        llvm::outs() << ")";
    }
    llvm::outs() << "\n";
    
    llvm::outs() << "    Total data: " << totalDataSize << " bytes\n";
    
    if (ctx.hasMemrefData(srcA) && ctx.hasMemrefData(srcB)) {
        auto& dataA = ctx.getMemrefData(srcA);
        auto& dataB = ctx.getMemrefData(srcB);
        
        size_t numElementsDst = getMemrefNumElements(dst);
        auto elementType = getElementType(dst);
        
        std::vector<uint8_t> resultData(getMemrefSize(dst), 0);
        
        llvm::outs() << "    Result [" << numElementsDst << " elements]: [simulated matmul result]\n";
        
        ctx.setMemrefData(dst, std::move(resultData));
    }
}

void executeHivmHirVmul(mlir::Operation* op, ExecutionContext& ctx) {
    auto operands = op->getOperands();
    
    if (operands.size() < 3) {
        llvm::outs() << "  [hivm.hir.vmul] Error: insufficient operands\n";
        return;
    }
    
    mlir::Value srcA = operands[0];
    mlir::Value srcB = operands[1];
    mlir::Value dst = operands[2];
    
    size_t dataSize = getMemrefSize(dst);
    size_t numElements = getMemrefNumElements(dst);
    auto elementType = getElementType(dst);
    
    llvm::outs() << "  [hivm.hir.vmul] Vector multiplication on VEC\n";
    llvm::outs() << "    Data size: " << dataSize << " bytes (" << numElements << " elements)\n";
    
    if (ctx.hasMemrefData(srcA) && ctx.hasMemrefData(srcB)) {
        auto& dataA = ctx.getMemrefData(srcA);
        auto& dataB = ctx.getMemrefData(srcB);
        
        std::vector<uint8_t> resultData(dataA.size());
        
        llvm::outs() << "    Result [" << numElements << " elements]: [";
        if (elementType.isF16()) {
            uint16_t* ptrA = reinterpret_cast<uint16_t*>(dataA.data());
            uint16_t* ptrB = reinterpret_cast<uint16_t*>(dataB.data());
            uint16_t* ptrR = reinterpret_cast<uint16_t*>(resultData.data());
            
            for (size_t i = 0; i < numElements; ++i) {
                llvm::APFloat a(llvm::APFloat::IEEEhalf(), llvm::APInt(16, ptrA[i]));
                llvm::APFloat b(llvm::APFloat::IEEEhalf(), llvm::APInt(16, ptrB[i]));
                a.multiply(b, llvm::APFloat::rmNearestTiesToEven);
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

void registerHivmInstructions() {
    ComponentLatencyModel mteModel;
    mteModel.baseLatency = 10;
    mteModel.bytesPerCycle = 128.0;
    mteModel.dataSizeDependent = true;
    
    ComponentLatencyModel vecModel;
    vecModel.baseLatency = 5;
    vecModel.bytesPerCycle = 256.0;
    vecModel.dataSizeDependent = true;
    
    ComponentLatencyModel cubeModel;
    cubeModel.baseLatency = 20;
    cubeModel.bytesPerCycle = 512.0;
    cubeModel.dataSizeDependent = true;
    
    REGISTER_INSTRUCTION_WITH_LATENCY("memref.alloc", executeMemrefAlloc, 1);
    
    REGISTER_INSTRUCTION_WITH_DATA_SIZE("hivm.hir.load", executeHivmHirLoad,
                                        calculateLoadStoreDataSize, mteModel,
                                        ExecutionUnitType::MTE);
    
    REGISTER_INSTRUCTION_WITH_DATA_SIZE("hivm.hir.store", executeHivmHirStore,
                                        calculateLoadStoreDataSize, mteModel,
                                        ExecutionUnitType::MTE);
    
    REGISTER_INSTRUCTION_WITH_DATA_SIZE("hivm.hir.vadd", executeHivmHirVadd,
                                        calculateVaddDataSize, vecModel,
                                        ExecutionUnitType::Vec);
    
    REGISTER_INSTRUCTION_WITH_DATA_SIZE("hivm.hir.vmul", executeHivmHirVmul,
                                        calculateVaddDataSize, vecModel,
                                        ExecutionUnitType::Vec);
    
    REGISTER_INSTRUCTION_WITH_DATA_SIZE("hivm.hir.matmul", executeHivmHirMatmul,
                                        calculateMatmulDataSize, cubeModel,
                                        ExecutionUnitType::Cube);
}

}
