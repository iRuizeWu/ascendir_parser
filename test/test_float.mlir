module {
  func.func @test_float() -> f32 {
    %c1_5 = arith.constant 1.5 : f32
    %c2_5 = arith.constant 2.5 : f32
    %sum = arith.addf %c1_5, %c2_5 : f32
    %diff = arith.subf %sum, %c1_5 : f32
    return %diff : f32
  }
}
