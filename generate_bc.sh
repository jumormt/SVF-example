$LLVM_DIR/bin/clang -S -c -Xclang -DINCLUDEMAIN -disable-O0-optnone -fno-discard-value-names -g -emit-llvm $1.c -o $1.ll
$LLVM_DIR/bin/opt -S -mem2reg $1.ll -o $1.ll
