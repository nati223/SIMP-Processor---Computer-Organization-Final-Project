add $t0, $zero, $imm, 1								# prepare to enable
out $t0, $zero, $imm, 1								# enable irq1
add $a0, $zero, $imm, 0                         	# set arbitrary sector (can be any number between 0-127)
add $a1, $zero, $imm, 1								# set arbitrary sector (can be any number between 0-127)
add $t1, $zero, $imm, wait							# prepare wait section to be irq1handler
out $t1, $zero, $imm, 6								# set wait as irq1handler

SEC0:
	in $t0, $zero, $imm, 17					    	# read diskstatus
	beq $imm, $t0, $zero, SEC0FREE				    # check if disk is free, if so branch to next section
	beq $imm, $zero, $zero, SEC0			    	# if busy, repeat SEC0 (loop)
SEC0FREE:
	add $t0, $zero, $imm , 1			     	    # set $t0 to read later
	out $a0, $imm, $zero, 15   						# IoRegisters[15] = sector0 (disksector)
    add $t1, $zero, $imm, 2048                      # prep IoRegisters[16] = 2048 (arbitrary diskbuffer)
	out $t1, $zero, $imm, 16   					    # IoRegisters[16] = 2048 (diskbuffer set)
	out $t0, $zero, $imm, 14     					# read Ioregisters[14] = 1
SEC1FREE:
	add $t0, $zero, $imm , 1			     	    # set to enable the read later
	out $a1, $imm, $zero, 15   						# IoRegisters[15] = sector1 (disksector)
    add $t1, $zero, $imm, 2176                      # prep IoRegisters[16] = 2176 (diskbuffer)
	out $t1, $zero, $imm, 16   					    # IoRegisters[16] = 2176 (diskbuffer set)
	out $t0, $zero, $imm, 14     					# read and Ioregisters[14] = 1
HDFREE:
	in $t1, $zero, $imm, 17							# read diskstatus
	bne $imm, $zero, $t1, HDFREE					# repeat until hard disk is free, so the program can continue
INITSUM:
	add $a0, $zero, $zero, 0						# index=0
	add $a1, $zero, $zero, 0						# sum0=0
	add $a2, $zero, $zero, 0 						# sum1=0
	add $t0, $zero, $imm, 8							# numbers to count
    add $t1, $zero, $imm, 2048						# set sector0
    add $t2, $zero, $imm, 2176						# set sector1
SUM_SEC:
	lw $s0, $zero, $t1, 0							# load from mem sector 0 to $s0
	lw $s1, $zero, $t2, 0							# load from mem sector 1 to $s1
	add $a1, $a1, $s0, 0							# aggregate sum of sector0
	add $a2, $a2 , $s1, 0							# aggregate sum of sector1
	add $a0, $a0, $imm , 1				            # i++ 
	add $t1, $t1, $imm, 1							# increment reading position
	add $t2, $t2, $imm, 1							# increment reading position
	blt $imm, $a0, $t0, SUM_SEC						# if i<8, continue the loop
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
	out $zero, $imm, $zero, 1						# disable irq1
	halt $zero, $zero, $zero, 0						# program is finished
wait:
	reti $zero, $zero, $zero, 0						# let reading\writing perform and return