module {
  func.func @test_arith(%a: i32, %b: i32) -> i32 {
    %sum = arith.addi %a, %b : i32
    %diff = arith.subi %sum, %b : i32
    return %diff : i32
  }
}
