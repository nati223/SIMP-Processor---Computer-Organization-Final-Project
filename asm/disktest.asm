
#label op,rd,rs,rt,imm
 
    add $sp, $sp, $imm, -8						    # open 2 places in our stack, 0
	sw $s0, $sp, $zero, 0						    # save s0 in stack, 2
	sw $s1, $sp, $imm, 4						    # save s1 in stack, 3
    beq $imm, $zero, $zero, main                    # go to main ,5
SEC0:
	in $t0, $zero, $imm, 17					    	# read diskstatus, 7
	beq $imm, $t0, $zero, SEC0FREE				    # check if disk is free, if so branch to next section, 9
	beq $imm, $zero, $zero, SEC0			    	# if busy, repeat SEC0 (loop), B
SEC0FREE:
	add $t0, $zero, $imm , 1			     	    # set to enable the read later, D
	out $a0, $imm, $zero, 15   						# IoRegisters[15] = sector0 (disksector), F
    add $t1, $zero, $imm, 2048                      # prep IoRegisters[16] = 2048 (arbitrary diskbuffer), 10
	out $t1, $zero, $imm, 16   					    # IoRegisters[16] = 0 (diskbuffer), 12
	out $t0, $zero, $imm, 14     					# read and Ioregisters[14] = 1, 14
SEC1:
	in $t0, $zero, $imm, 17 						# read diskstatus, 16
	beq $imm, $t0, $zero, SEC1FREE  				# check if disk is free, if so branch to next section, 18
	beq $imm, $zero, $zero, SEC1 			    	# if busy, repeat SEC1 (loop) else, continue, 1A
SEC1FREE:
	add $t0, $zero, $imm , 1			     	    # set to enable the read later, 1C
	out $a1, $imm, $zero, 15   						# IoRegisters[15] = sector1 (disksector), 1E
    add $t1, $zero, $imm, 2176                      # prep IoRegisters[16] = 2176 (diskbuffer), 20
	out $t1, $zero, $imm, 16   					    # IoRegisters[16] = 2176 (diskbuffer), 22
	out $t0, $zero, $imm, 14     					# read and Ioregisters[14] = 1, 24
HDFREE:
	in $t1, $zero, $imm, 17							# read diskstatus, 26
	bne $imm, $zero, $t1, HDFREE					# repeat until hard disk is free, so the program can continue, 28
INITSUM:
	add $a0, $zero, $zero, 0						# index=0, 2A
	add $a1, $zero, $zero, 0						# sum0=0, 2B
	add $a2, $zero, zero, 0 						# sum1=0 , 2C
	add $t0, $zero, $imm, 8							# numbers to count, 2D
    add $t1, $zero, $imm, 2048						# set sector0, 2F
    add $t2, $zero, $imm, 2176						# set sector1, 31
SUM_SEC:
	lw $s0, $zero, $t1, 0							#load from mem sector 0 to $s0 ,33
	lw $s1, $zero, $t2, 0							#load from mem sector 1 to $s1 ,34
	add $a1, $a1, $s0, 0							# 35
	add $a2, $a2 , $s1, 0							# 36
	add $a0, $a0, $imm , 1				            # i++ ,37
	add $t1, $t1, $imm, 1							# increment reading position, 39
	add $t2, $t2, $imm, 1							# increment reading position, 3B
	blt $imm, $a0, $t0, SUM_SEC						# if i<8, continue the loop, 3D
STORE:
    sw $a1, $zero, $imm, 256						# store sector0 in 0x100
    sw $a2, $zero, $imm, 257						# store sector1 in 0x101
    bgt $imm, $a2, $a1, op2							# brach if sector1 sum > sector0 sum
op1:
    sw $a1, $zero, $imm, 258						# store in 0x102 sector0 sum
    beq $imm, $zero, $zero, END						# Go to end of program
op2:
    sw $a2, $zero, $imm, 258						# store in 0x102 sector1 sum
END:
	lw $s0, $sp, $zero, 0							# restore $s0, 4B
	lw $s1, $sp, $imm, 4							# restore $s1, 4C
	add $sp, $sp, $imm, 8							# load 2 places back, 4E
	halt $zero, $zero, $zero, 0						# program is finished, 50
main:
    add $a0, $zero, $imm, 0                         # set arbitrary sector
    add $a1, $zero, $imm, 1							# set arbitrary sector
    beq $imm, $zero, $zero, SEC0					