module {
  func.func @test_constant() -> i32 {
    %c42 = arith.constant 42 : i32
    return %c42 : i32
  }
}
