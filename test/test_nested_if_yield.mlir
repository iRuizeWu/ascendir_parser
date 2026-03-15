module {
  func.func @test_nested_if_yield(%arg0: i1, %arg1: i1) -> i32 {
    %c0 = arith.constant 0 : i32
    %c1 = arith.constant 1 : i32
    %c5 = arith.constant 5 : i32
    %c10 = arith.constant 10 : i32
    
    %result = scf.if %arg0 -> i32 {
      %inner = scf.if %arg1 -> i32 {
        scf.yield %c10 : i32
      } else {
        scf.yield %c5 : i32
      }
      scf.yield %inner : i32
    } else {
      scf.yield %c0 : i32
    }
    
    return %result : i32
  }
}
