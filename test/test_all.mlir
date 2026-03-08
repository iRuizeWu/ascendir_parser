module {
  func.func @test_all() -> i32 {
    %c10 = arith.constant 10 : i32
    %c20 = arith.constant 20 : i32
    %c30 = arith.constant 30 : i32
    
    %sum = arith.addi %c10, %c20 : i32
    %diff = arith.subi %sum, %c10 : i32
    %prod = arith.muli %diff, %c10 : i32
    %quot = arith.divsi %prod, %c10 : i32
    
    %cond = arith.cmpi slt, %c10, %c20 : i32
    
    return %quot : i32
  }
}
