module {
  func.func @test_nested_for() -> i32 {
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c3 = arith.constant 3 : index
    %c0_i32 = arith.constant 0 : i32
    
    %sum = scf.for %i = %c0 to %c3 step %c1 
        iter_args(%acc_i = %c0_i32) -> (i32) {
      %sum_j = scf.for %j = %c0 to %c3 step %c1 
          iter_args(%acc_j = %acc_i) -> (i32) {
        %idx_i = arith.index_cast %i : index to i32
        %idx_j = arith.index_cast %j : index to i32
        %mul = arith.muli %idx_i, %idx_j : i32
        %new_acc = arith.addi %acc_j, %mul : i32
        scf.yield %new_acc : i32
      }
      scf.yield %sum_j : i32
    }
    return %sum : i32
  }
}
