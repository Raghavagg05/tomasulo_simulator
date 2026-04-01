.A: 1 2 3
.B: 4 5 6

# a comment
loop: add x1, x2, x3
    lw x4 B(x1)
    sw x5 A(x1)
    addi x1, x1, 1
    beq x1, x2, loop
    j loop