module {
  func.func @test_constants() -> i32 {
    %c10 = arith.constant 10 : i32
    %c20 = arith.constant 20 : i32
    %sum = arith.addi %c10, %c20 : i32
    %diff = arith.subi %sum, %c10 : i32
    %prod = arith.muli %diff, %c10 : i32
    return %prod : i32
  }
}
