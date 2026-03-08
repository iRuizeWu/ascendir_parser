module {
  func.func @test_basic__kernel0(%valueA: memref<16xf16>,
                                 %valueB: memref<16xf16>,
                                 %valueC: memref<16xf16>)
  {
    %ubA = memref.alloc() : memref<16xf16>
    "hivm.hir.load"(%valueA, %ubA) : (memref<16xf16>, memref<16xf16>) -> ()

    %ubB = memref.alloc() : memref<16xf16>
    "hivm.hir.load"(%valueB, %ubB) : (memref<16xf16>, memref<16xf16>) -> ()

    %ubC = memref.alloc() : memref<16xf16>
    "hivm.hir.vadd"(%ubA, %ubB, %ubC) : (memref<16xf16>, memref<16xf16>, memref<16xf16>) -> ()
    
    "hivm.hir.store"(%ubC, %valueC) : (memref<16xf16>, memref<16xf16>) -> ()
    
    return
  }
}
