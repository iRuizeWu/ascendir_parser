module {
  func.func @test_pipeline_execution(%inputA: memref<256xf16>,
                                      %inputB: memref<256xf16>,
                                      %inputC: memref<256xf16>,
                                      %output: memref<256xf16>)
  {
    %ubA = memref.alloc() : memref<256xf16>
    "hivm.hir.load"(%inputA, %ubA) : (memref<256xf16>, memref<256xf16>) -> ()

    %ubB = memref.alloc() : memref<256xf16>
    "hivm.hir.load"(%inputB, %ubB) : (memref<256xf16>, memref<256xf16>) -> ()
    
    %ubC = memref.alloc() : memref<256xf16>
    "hivm.hir.load"(%inputC, %ubC) : (memref<256xf16>, memref<256xf16>) -> ()

    %mulResult = memref.alloc() : memref<256xf16>
    "hivm.hir.vmul"(%ubA, %ubB, %mulResult) : (memref<256xf16>, memref<256xf16>, memref<256xf16>) -> ()
    
    %addResult = memref.alloc() : memref<256xf16>
    "hivm.hir.vadd"(%mulResult, %ubC, %addResult) : (memref<256xf16>, memref<256xf16>, memref<256xf16>) -> ()
    
    "hivm.hir.store"(%addResult, %output) : (memref<256xf16>, memref<256xf16>) -> ()
    
    return
  }
}
