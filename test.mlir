module {
  func.func @add(%arg0: i32, %arg1: i32) -> i32 {
    %result = arith.addi %arg0, %arg1 : i32
    func.return %result : i32
  }
}
