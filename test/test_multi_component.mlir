module {
  func.func @test_multi_component(%valueA: memref<128xf16>,
                                   %valueB: memref<128xf16>,
                                   %valueC: memref<128xf16>,
                                   %valueD: memref<128xf16>,
                                   %output: memref<128xf16>)
  {
    %ubA = memref.alloc() : memref<128xf16>
    "hivm.hir.load"(%valueA, %ubA) : (memref<128xf16>, memref<128xf16>) -> ()

    %ubB = memref.alloc() : memref<128xf16>
    "hivm.hir.load"(%valueB, %ubB) : (memref<128xf16>, memref<128xf16>) -> ()

    %ubC = memref.alloc() : memref<128xf16>
    "hivm.hir.vadd"(%ubA, %ubB, %ubC) : (memref<128xf16>, memref<128xf16>, memref<128xf16>) -> ()
    
    %ubD = memref.alloc() : memref<128xf16>
    "hivm.hir.load"(%valueC, %ubD) : (memref<128xf16>, memref<128xf16>) -> ()
    
    %ubE = memref.alloc() : memref<128xf16>
    "hivm.hir.vadd"(%ubC, %ubD, %ubE) : (memref<128xf16>, memref<128xf16>, memref<128xf16>) -> ()
    
    "hivm.hir.store"(%ubE, %output) : (memref<128xf16>, memref<128xf16>) -> ()
    
    return
  }
}
