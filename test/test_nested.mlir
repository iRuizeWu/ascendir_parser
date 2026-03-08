module {
  func.func @test_nested() -> i32 {
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c10 = arith.constant 10 : index
    %c2 = arith.constant 2 : index
    %c0_i32 = arith.constant 0 : i32
    
    %sum = scf.for %i = %c0 to %c10 step %c1 
        iter_args(%acc = %c0_i32) -> (i32) {
      %cond = arith.cmpi sgt, %i, %c2 : index
      %val = scf.if %cond -> (i32) {
        %idx_i32 = arith.index_cast %i : index to i32
        scf.yield %idx_i32 : i32
      } else {
        scf.yield %c0_i32 : i32
      }
      %new_acc = arith.addi %acc, %val : i32
      scf.yield %new_acc : i32
    }
    return %sum : i32
  }
}
