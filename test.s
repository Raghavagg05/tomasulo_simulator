.DATA 1 2 3 4 5 6
add x1 x2 x3
lw x4 3(x1)
sw x5 0(x1)
addi x1 x1 1
beq x1 x2 -4
j -5
