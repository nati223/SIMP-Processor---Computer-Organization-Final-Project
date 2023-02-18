	add $t2, $zero, $imm, 1				# $t2 = 1 ,000
	out $t2, $zero, $imm, 2				# enable irq2 ,002
	sll $sp, $t2, $imm, 11				# set $sp = 1 << 11 = 2048 ,004
	add $t2, $zero, $imm, L3			# $t1 = address of L3, 006
	out $t2, $zero, $imm, 6				# set irqhandler as L3, 008
	lw $a0, $zero, $imm, 256			# get x from address 256, 00A
	jal $ra, $imm, $zero, fib			# calc $v0 = fib(x) , 00C
	sw $v0, $zero, $imm, 257			# store fib(x) in 257, 00E
	halt $zero, $zero, $zero, 0			# halt , 010
fib:
	add $sp, $sp, $imm, -3				# adjust stack for 3 items, line 011
	sw $s0, $sp, $imm, 2				# save $s0 , 013
	sw $ra, $sp, $imm, 1				# save return address , 015
	sw $a0, $sp, $imm, 0				# save argument , 017
	add $t2, $zero, $imm, 1				# $t2 = 1 , 019
	bgt $imm, $a0, $t2, L1				# jump to L1 if x > 1 ,01B
	add $v0, $a0, $zero, 0				# otherwise, fib(x) = x, copy input ,01D
	beq $imm, $zero, $zero, L2			# jump to L2 ,01E
L1:
	sub $a0, $a0, $imm, 1				# calculate x - 1, 020
	jal $ra, $imm, $zero, fib			# calc $v0=fib(x-1) , 022
	add $s0, $v0, $zero, 0				# $s0 = fib(x-1), 024
	lw $a0, $sp, $imm, 0				# restore $a0 = x, ,025
	sub $a0, $a0, $imm, 2				# calculate x - 2, 027
	jal $ra, $imm, $zero, fib			# calc fib(x-2), 029
	add $v0, $v0, $s0, 0				# $v0 = fib(x-2) + fib(x-1), 02B
	lw $a0, $sp, $imm, 0				# restore $a0, 02C
	lw $ra, $sp, $imm, 1				# restore $ra, 02E
	lw $s0, $sp, $imm, 2				# restore $s0, 030
L2:
	add $sp, $sp, $imm, 3				# pop 3 items from stack, 032
	add $t0, $a0, $zero, 0				# $t0 = $a0, 034
	sll $t0, $t0, $imm, 16				# $t0 = $t0 << 16, 035
	add $t0, $t0, $v0, 0				# $t0 = $t0 + $v0, 037
	out $t0, $zero, $imm, 10			# write $t0 to display, 038
	beq $ra, $zero, $zero, 0			# and return, 03A
L3:
	in $t1, $zero, $imm, 9				# read leds register into $t1, 03B
	sll $t1, $t1, $imm, 1				# left shift led pattern to the left, 03D
	or $t1, $t1, $imm, 1				# lit up the rightmost led, 03F
	out $t1, $zero, $imm, 9				# write the new led pattern, 041
	add $t1, $zero, $imm, 255			# $t1 = 255, 043
	out $t1, $zero, $imm, 21			# set pixel color to white, 043
	add $t1, $zero, $imm, 1				# $t1 = 1, 045
	out $t1, $zero, $imm, 22			# draw pixel, 047
	in $t1, $zero, $imm, 20				# read pixel address, 049
	add $t1, $t1, $imm, 257				# $t1 += 257, 04B
	out $t1, $zero, $imm, 20			# update address, 04D
	out $zero, $zero, $imm, 5			# clear irq2 status, 04F
	reti $zero, $zero, $zero, 0			# return from interrupt ,050
	.word 256 7
