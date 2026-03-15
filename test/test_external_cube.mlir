module {
  func.func private @external_lib.matmul(%arg0: memref<64x64xf16>, %arg1: memref<64x64xf16>, %arg2: memref<64x64xf16>)
  
  func.func @test_external_with_cube() {
    %inputA = memref.alloc() : memref<64x64xf16>
    %inputB = memref.alloc() : memref<64x64xf16>
    %output = memref.alloc() : memref<64x64xf16>
    
    func.call @external_lib.matmul(%inputA, %inputB, %output) : (memref<64x64xf16>, memref<64x64xf16>, memref<64x64xf16>) -> ()
    
    return
  }
}
