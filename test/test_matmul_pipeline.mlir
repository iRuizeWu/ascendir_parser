module {
  func.func @test_matmul_pipeline(%matrixA: memref<64x64xf16>,
                                   %matrixB: memref<64x64xf16>,
                                   %matrixC: memref<64x64xf16>,
                                   %output: memref<64x64xf16>)
  {
    %ubA = memref.alloc() : memref<64x64xf16>
    "hivm.hir.load"(%matrixA, %ubA) : (memref<64x64xf16>, memref<64x64xf16>) -> ()

    %ubB = memref.alloc() : memref<64x64xf16>
    "hivm.hir.load"(%matrixB, %ubB) : (memref<64x64xf16>, memref<64x64xf16>) -> ()

    %ubC = memref.alloc() : memref<64x64xf16>
    "hivm.hir.load"(%matrixC, %ubC) : (memref<64x64xf16>, memref<64x64xf16>) -> ()
    
    %matmulResult = memref.alloc() : memref<64x64xf16>
    "hivm.hir.matmul"(%ubA, %ubB, %matmulResult) : (memref<64x64xf16>, memref<64x64xf16>, memref<64x64xf16>) -> ()
    
    %addResult = memref.alloc() : memref<64x64xf16>
    "hivm.hir.vadd"(%matmulResult, %ubC, %addResult) : (memref<64x64xf16>, memref<64x64xf16>, memref<64x64xf16>) -> ()
    
    "hivm.hir.store"(%addResult, %output) : (memref<64x64xf16>, memref<64x64xf16>) -> ()
    
    return
  }
}
