module {
  func.func @test_hivm_complex(%valueA: memref<32xf16>,
                               %valueB: memref<32xf16>,
                               %valueC: memref<32xf16>)
  {
    %ubA = memref.alloc() : memref<32xf16>
    "hivm.hir.load"(%valueA, %ubA) : (memref<32xf16>, memref<32xf16>) -> ()

    %ubB = memref.alloc() : memref<32xf16>
    "hivm.hir.load"(%valueB, %ubB) : (memref<32xf16>, memref<32xf16>) -> ()

    %ubC = memref.alloc() : memref<32xf16>
    "hivm.hir.vadd"(%ubA, %ubB, %ubC) : (memref<32xf16>, memref<32xf16>, memref<32xf16>) -> ()
    
    "hivm.hir.store"(%ubC, %valueC) : (memref<32xf16>, memref<32xf16>) -> ()
    
    return
  }
}
