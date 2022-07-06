li s0 0x80000
li a4 1
li x1 16
li x5 0x10
li x6 4
sw x5 0xc(s0)
loop:
lw a3 0x8(s0)
and x3 a3 x1
srl x8 x3 x6
beq a4 x8 exit
jal loop
exit:

lw a5 0x0(s0)
sw a5 0x4(s0)

rep:
jal rep
