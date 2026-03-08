module {
  func.func @test_for_simple() -> i32 {
    %c0_i32 = arith.constant 0 : i32
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c5 = arith.constant 5 : index
    
    %sum_idx = scf.for %i = %c0 to %c5 step %c1 
        iter_args(%acc = %c0) -> (index) {
      %new_acc = arith.addi %acc, %i : index
      scf.yield %new_acc : index
    }
    %sum = arith.index_cast %sum_idx : index to i32
    return %sum : i32
  }
}
