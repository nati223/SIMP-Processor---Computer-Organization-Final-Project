add $t0, $imm, $zero, 800				# $t0= arbitrary parameter
add $t1, $imm, $zero, 30				# $t1=arbitrary parameter
sw $t0, $imm, $zero, 0x100				# save the value $t0 in address 0x100
sw $t1, $imm, $zero, 0x101				# save the value $t0 in address 0x101

draw:
lw $s0, $imm, $zero, 0x100				# load the upper-left corner from 0x100 to $s0, $s0 is the upper left-hand corner
lw $s1, $imm, $zero, 0x101				# load the length from 0x101 to $s1, $s1 is the length
mul $t1, $imm, $s1, 256					# calculate 256 mul the length of the square 
add $t1, $s0, $t1, 0					# calculate the lower left point in the square
add $t2, $imm, $zero, 65535				# $t2=65535
bgt $imm, $t1, $t2, end					# if $t1>$t2, jump to end. the square is out of border
add $t2, $zero, $zero, 0				# $t2=0, represents the row
add $t0, $imm, $zero, 255				# $t0 = 255
add $t1, $s0, $zero, 0					# $t1 = $s1 => the upper left corner
bgt $imm, $t1, $t0, line				# if the upper left point bigger than

line:
	add $t2, $imm, $t2, 1				# $t2=1, represents the row
	sub $t1, $t1, $imm, 256				# $t0 = $t0-256
	bgt $imm, $t1, $t0, line			# if the upper left point bigger than 

mul $t2, $t2, $imm, 256					# $t2 = 256*num of row
add $t2, $t2, $imm, 255					# right border
add $t0, $s1, $s0, 0					# the right corner of the square
bgt $imm, $t0, $t2, end					# if $t0>$t2, jump to end

add $t0, $imm, $zero, 255				# $t0 = 255
out $t0, $imm, $zero, 21				# IORegiste[21] = 255, monitordata set on white
add $t1, $imm, $zero, 1					# $t1 = 1
add $a3, $s1, $zero, 0					# $a3 = column left to fill
add $a0, $s0, $zero, 0					# copy $s1 to $a0, the current place, runs on the columns
add $a2, $s1, $zero, 0					# $a2 = rows left to fill

loop_column:
	out $a0, $imm, $zero, 20			# IORegiste[20] = $a0, monitoraddr set on the upper left-hand corner
	out $t1, $imm, $zero, 22			# IORegiste[22] = 1, monitorcmd set on 1, write pixel monitor
	add $a0, $imm, $a0, 1				# $a0 = $a0+1
	add $a3, $a3, $imm, -1				# $a3 = length left to fill
	bge $imm, $a3, $zero, loop_column	# repeat this loop length times 

loop_row:
	add $a3, $s1, $zero, 0				# $a3 = rewind the length left to fill
	add $a2, $imm, $a2, -1				# $a2 is counting the num of loops
	sub $a0, $a0, $s1, 0				# rewind $a0 to the second column
	sub $a0, $a0, $imm, 1				# rewind $a0 to the first column
	add $a0, $a0, $imm, 256				# update $a0 to the next row
	bge $imm, $a2, $zero, loop_column	# repeat this loop length times 

end:
	out $zero, $imm, $zero, 22			# IORegiste[22] = 0, monitorcmd set on 00
	lw $s0, $sp, $zero, 0				# restore $s0
	lw $s1, $sp, $imm, 1				# restore $s1
	add $sp, $sp, $imm, 2				# load 2 places back
	halt $zero, $zero, $zero, 0