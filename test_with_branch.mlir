module @ComprehensiveTestWithBranch {
  llvm.func @llvm.hivm.SET.FLAG.IMM(i64, i64, i64)
  llvm.func @llvm.hivm.SET.CTRL(i64)
  llvm.func @llvm.hivm.GET.CTRL() -> i64
  llvm.func @llvm.hivm.SBITSET1(i64, i64) -> i64
  llvm.func @llvm.hivm.GET.BLOCK.IDX() -> i64

  llvm.func @test_with_branch(%arg0: i32, %arg1: i32, %arg2: i64, %arg3: i64, %arg4: i64, %arg5: i32) -> i64 {
    %c1_i64 = llvm.mlir.constant(1 : i64) : i64
    %c4_i64 = llvm.mlir.constant(4 : i64) : i64
    %c5_i64 = llvm.mlir.constant(5 : i64) : i64
    %c0_i64 = llvm.mlir.constant(0 : i64) : i64
    %c0_i32 = llvm.mlir.constant(0 : i32) : i32
    
    %26 = llvm.add %arg0, %arg1 : i32
    %27 = llvm.intr.smin(%26, %arg5) : (i32, i32) -> i32
    %28 = llvm.sext %27 : i32 to i64
    
    llvm.call @llvm.hivm.SET.FLAG.IMM(%c1_i64, %c1_i64, %c4_i64) : (i64, i64, i64) -> ()
    llvm.call @llvm.hivm.SET.FLAG.IMM(%c0_i64, %c5_i64, %c1_i64) : (i64, i64, i64) -> ()
    
    %cond = llvm.icmp "slt" %27, %c0_i32 : i32
    llvm.cond_br %cond, ^bb1, ^bb2
    
  ^bb1:
    %16 = llvm.add %arg2, %arg3 : i64
    llvm.call @llvm.hivm.SET.CTRL(%16) : (i64) -> ()
    llvm.br ^bb3(%16 : i64)
    
  ^bb2:
    llvm.call @llvm.hivm.SET.CTRL(%arg4) : (i64) -> ()
    llvm.br ^bb3(%arg4 : i64)
    
  ^bb3(%val: i64):
    %17 = llvm.call @llvm.hivm.GET.CTRL() : () -> i64
    %18 = llvm.call @llvm.hivm.SBITSET1(%17, %arg4) : (i64, i64) -> i64
    llvm.call @llvm.hivm.SET.CTRL(%18) : (i64) -> ()
    
    %19 = llvm.call @llvm.hivm.GET.BLOCK.IDX() : () -> i64
    %20 = llvm.add %19, %28 : i64
    
    llvm.return %20 : i64
  }
}
