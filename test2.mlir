module {
  func.func @nested(%arg0: i32) -> i32 {
    %c1 = arith.constant 1 : i32
    %c2 = arith.constant 2 : i32
    %sum = arith.addi %c1, %c2 : i32
    %result = arith.muli %sum, %arg0 : i32
    func.return %result : i32
  }
}
