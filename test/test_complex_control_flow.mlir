module {
  func.func @test_complex_control_flow() -> i32 {
    %c0 = arith.constant 0 : index
    %c1 = arith.constant 1 : index
    %c4 = arith.constant 4 : index
    %c0_i32 = arith.constant 0 : i32
    %c2_idx = arith.constant 2 : index
    %c2_i32 = arith.constant 2 : i32
    
    %sum = scf.for %i = %c0 to %c4 step %c1 
        iter_args(%acc = %c0_i32) -> (i32) {
      %idx_i = arith.index_cast %i : index to i32
      
      %cond_even = arith.cmpi eq, %i, %c0 : index
      %is_even = scf.if %cond_even -> (i1) {
        %mod = arith.constant false
        scf.yield %mod : i1
      } else {
        %two = arith.constant 2 : index
        %div = arith.divsi %i, %two : index
        %mul = arith.muli %div, %two : index
        %is_div = arith.cmpi eq, %mul, %i : index
        scf.yield %is_div : i1
      }
      
      %val = scf.if %is_even -> (i32) {
        %double = arith.muli %idx_i, %c2_i32 : i32
        scf.yield %double : i32
      } else {
        %triple = arith.muli %idx_i, %c2_i32 : i32
        scf.yield %triple : i32
      }
      
      %new_acc = arith.addi %acc, %val : i32
      scf.yield %new_acc : i32
    }
    return %sum : i32
  }
}
