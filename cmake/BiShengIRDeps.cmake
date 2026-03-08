# BiShengIR Dependencies Configuration
# This file centralizes all required libraries for BiShengIR support

set(BISHENGIR_DIALECT_LIBS
    BiShengIRHIVMDialect
    BiShengIRHFusionDialect
    BiShengIRHACCDialect
    BiShengIRAnnotationDialect
    BiShengIRMemRefExtDialect
    BiShengIRScopeDialect
    BiShengIRSymbolDialect
    BiShengIRMathExtDialect
    BiShengIRDialectUtils
    BiShengIRMemRefDialect
    BiShengIRMemRefTransforms
    BiShengIRLinalgDialectExt
    BiShengIRTensorDialect
    BiShengIRTensorUtils
    CACHE INTERNAL "BiShengIR dialect libraries"
)

set(MLIR_DIALECT_LIBS
    MLIRLinalgDialect
    MLIRSCFDialect
    MLIRTensorDialect
    MLIRTensorUtils
    MLIRMathDialect
    MLIRAffineDialect
    MLIRBufferizationDialect
    MLIRVectorDialect
    MLIRLLVMDialect
    MLIRComplexDialect
    MLIRGPUDialect
    MLIRSPIRVDialect
    MLIRAsyncDialect
    MLIRTransformDialect
    MLIRMemRefTransforms
    MLIRAffineTransforms
    MLIRAffineUtils
    MLIRTensorTransforms
    CACHE INTERNAL "MLIR dialect libraries required by BiShengIR"
)

function(link_bishengir target)
    target_link_libraries(${target} PRIVATE
        ${BISHENGIR_DIALECT_LIBS}
        ${MLIR_DIALECT_LIBS}
    )
endfunction()