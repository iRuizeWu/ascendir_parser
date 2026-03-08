module {
  func.func @test_nested_if() -> i32 {
    %c1 = arith.constant 1 : i32
    %c2 = arith.constant 2 : i32
    %c3 = arith.constant 3 : i32
    %c5 = arith.constant 5 : i32
    %c10 = arith.constant 10 : i32
    
    %cond1 = arith.cmpi sgt, %c10, %c5 : i32
    %result = scf.if %cond1 -> (i32) {
      %cond2 = arith.cmpi sgt, %c5, %c3 : i32
      %inner = scf.if %cond2 -> (i32) {
        %cond3 = arith.cmpi sgt, %c3, %c2 : i32
        %deep = scf.if %cond3 -> (i32) {
          scf.yield %c1 : i32
        } else {
          scf.yield %c2 : i32
        }
        scf.yield %deep : i32
      } else {
        scf.yield %c3 : i32
      }
      scf.yield %inner : i32
    } else {
      scf.yield %c10 : i32
    }
    return %result : i32
  }
}
