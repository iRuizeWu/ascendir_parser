module {
  func.func @test_all_intr_operations(%arg0: i32, %arg1: i32, %arg2: i64, %arg3: i64, %arg4: i64, %arg5: i32) -> i64 {
    %c1_i64 = arith.constant 1 : i64
    %c4_i64 = arith.constant 4 : i64
    %c5_i64 = arith.constant 5 : i64
    %c0_i64 = arith.constant 0 : i64
    
    %26 = arith.addi %arg0, %arg1 : i32
    %27 = llvm.intr.smin(%26, %arg5) : (i32, i32) -> i32
    %28 = llvm.sext %27 : i32 to i64
    
    "hivm.intr.hivm.SET.FLAG.IMM"() <{event_id = 1 : i64, set_pipe = 1 : i64, wait_pipe = 4 : i64}> : () -> ()
    "hivm.intr.hivm.SET.FLAG.IMM"() <{event_id = 0 : i64, set_pipe = 5 : i64, wait_pipe = 1 : i64}> : () -> ()
    
    cf.br ^bb1
    
  ^bb1:
    %16 = arith.addi %arg2, %arg3 : i64
    "hivm.intr.hivm.SET.CTRL"(%16) : (i64) -> ()
    %17 = "hivm.intr.hivm.GET.CTRL"() : () -> i64
    %18 = "hivm.intr.hivm.SBITSET1"(%17, %arg4) : (i64, i64) -> i64
    "hivm.intr.hivm.SET.CTRL"(%18) : (i64) -> ()
    %19 = "hivm.intr.hivm.GET.BLOCK.IDX"() : () -> i64
    
    %result = arith.addi %19, %28 : i64
    func.return %result : i64
  }
}
