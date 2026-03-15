module {
  func.func private @external_matmul(%arg0: memref<64x64xf16>, %arg1: memref<64x64xf16>, %arg2: memref<64x64xf16>)
  
  func.func private @external_conv2d(%arg0: memref<64x64xf16>, %arg1: memref<64x64xf16>, %arg2: memref<64x64xf16>)
  
  func.func @test_external_call() {
    %inputA = memref.alloc() : memref<64x64xf16>
    %inputB = memref.alloc() : memref<64x64xf16>
    %output = memref.alloc() : memref<64x64xf16>
    
    func.call @external_matmul(%inputA, %inputB, %output) : (memref<64x64xf16>, memref<64x64xf16>, memref<64x64xf16>) -> ()
    
    %filter = memref.alloc() : memref<64x64xf16>
    %convOut = memref.alloc() : memref<64x64xf16>
    
    func.call @external_conv2d(%output, %filter, %convOut) : (memref<64x64xf16>, memref<64x64xf16>, memref<64x64xf16>) -> ()
    
    return
  }
}
