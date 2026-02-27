module @MemRefTest {
  func.func @alloc_and_use(%arg0: index, %arg1: index) -> memref<?x?xf32> {
    %0 = memref.alloc(%arg0, %arg1) : memref<?x?xf32>
    return %0 : memref<?x?xf32>
  }

  func.func @load_store(%arg0: memref<10xf32>, %arg1: index) -> f32 {
    %0 = memref.load %arg0[%arg1] : memref<10xf32>
    %cst = arith.constant 1.0 : f32
    %1 = arith.addf %0, %cst : f32
    memref.store %1, %arg0[%arg1] : memref<10xf32>
    return %1 : f32
  }
}
