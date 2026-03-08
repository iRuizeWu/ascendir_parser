module {
  func.func @test_deep_nesting() -> i32 {
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c2 = arith.constant 2 : index
    %c0_i32 = arith.constant 0 : i32
    
    %sum = scf.for %i = %c0 to %c2 step %c1 
        iter_args(%acc_i = %c0_i32) -> (i32) {
      %sum_j = scf.for %j = %c0 to %c2 step %c1 
          iter_args(%acc_j = %acc_i) -> (i32) {
        %idx_i = arith.index_cast %i : index to i32
        %idx_j = arith.index_cast %j : index to i32
        
        %cond_i = arith.cmpi sgt, %i, %c0 : index
        %val_ij = scf.if %cond_i -> (i32) {
          %cond_j = arith.cmpi sgt, %j, %c0 : index
          %inner = scf.if %cond_j -> (i32) {
            %mul = arith.muli %idx_i, %idx_j : i32
            scf.yield %mul : i32
          } else {
            scf.yield %idx_i : i32
          }
          scf.yield %inner : i32
        } else {
          scf.yield %idx_j : i32
        }
        
        %new_acc = arith.addi %acc_j, %val_ij : i32
        scf.yield %new_acc : i32
      }
      scf.yield %sum_j : i32
    }
    return %sum : i32
  }
}
