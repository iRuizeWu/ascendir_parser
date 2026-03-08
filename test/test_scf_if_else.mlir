module {
  func.func @test_if_else() -> i32 {
    %c1 = arith.constant 1 : i32
    %c2 = arith.constant 2 : i32
    %c10 = arith.constant 10 : i32
    %cond = arith.cmpi slt, %c10, %c2 : i32
    
    %result = scf.if %cond -> (i32) {
      scf.yield %c1 : i32
    } else {
      scf.yield %c2 : i32
    }
    return %result : i32
  }
}
