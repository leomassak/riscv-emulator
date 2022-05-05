.text
.global _start; _start:

addi x11,zero,0


addi x10,zero,10


addi x10,x10,2
addi x1,zero,1
addi x2,zero,1
addi x3,zero,2
addi x11,zero,2

loop:
add x2,x2,x1
addi x11,x11,1
add x20,zero,x2
beq x11,x10,end
add x1,x2,x1
addi x11,x11,1
add x20,zero,x1
bne x11,x10,loop
end:
