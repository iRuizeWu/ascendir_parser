module {
  func.func @test_compare() -> i1 {
    %c10 = arith.constant 10 : i32
    %c20 = arith.constant 20 : i32
    %cond = arith.cmpi slt, %c10, %c20 : i32
    return %cond : i1
  }
}
