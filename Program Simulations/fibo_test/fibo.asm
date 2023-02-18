add $sp, $sp, $imm, -2		           # adjust stack for 2 items, 0
sw $s0, $sp, $imm, 1                  # save the previous number, 2
sw $s1, $sp, $zero, 0                 # save the current number, 4
add $t0, $imm, $zero, 256             # $t0 will hold initial storing address 0x100, 5

# starting condition
add $s0, $zero, $zero, 0                # set s0 to 0, 6
add $s1, $zero, $imm, 1                 # set s1 to 1, 7
sw $s0, $zero, $t0, 0                   # store s0, 9
add $t0, $t0, $imm, 1                   # increment addresss, A
sw $s1, $zero, $t0, 0                   # store s1, C
add $t0, $t0, $imm, 1                   # increment address, D
add $t1, $zero, $imm, 0x7FFFF           # stopping value for the loop

fib_loop: # Start the loop to calculate the Fibonacci sequence
    
    add $t2, $s0, $s1, 0              # $t2=$s0+$s1, Calculate the next number in the sequence by adding the previous and current numbers, F
    bgt $imm, $t2, $t1, end           # stopping condition
    add $s0, $zero, $s1, 0            # $s0 now stores the previous number, 10
    add $s1, $t2, $zero,0             # $s1 now stores the current number, 11

    sw $s1, $zero, $t0                # store in address, 12
    add $t0, $t0, $imm, 1             # increment address, 13
    beq $imm, $zero, $zero, fib_loop  # jump to fib_loop, 15

end:
	lw $s0, $sp, $zero, 0							# restore $s0, 4B
	lw $s1, $sp, $imm, 1							# restore $s1, 4C
    add $sp, $sp, $imm, 2							# load 2 places back, 4E
    halt $zero, $zero, $zero, 0