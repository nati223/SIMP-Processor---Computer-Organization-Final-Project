add $t0, $imm, $zero, 256               # $t0 will hold initial storing address 0x100

# starting condition
add $s0, $zero, $zero, 0                # set s0 to 0
add $s1, $zero, $imm, 1                 # set s1 to 1
sw $s0, $zero, $t0, 0                   # store s0
add $t0, $t0, $imm, 1                   # increment addresss
sw $s1, $zero, $t0, 0                   # store s1
add $t0, $t0, $imm, 1                   # increment address
add $t1, $zero, $imm, 0x7FFFF           # stopping value for the loop

fib_loop: # Start the loop to calculate the Fibonacci sequence
    
    add $t2, $s0, $s1, 0                # $t2=$s0+$s1, Calculate the next number in the sequence by adding the previous and current
    bgt $imm, $t2, $t1, end             # Check if overflow occurs
    add $s0, $zero, $s1, 0              # $s0 now stores the previous number
    add $s1, $t2, $zero,0               # $s1 now stores the current number

    sw $s1, $zero, $t0, 0               # store in address
    add $t0, $t0, $imm, 1               # increment address
    beq $imm, $zero, $zero, fib_loop    # jump to fib_loop

end:
    halt $zero, $zero, $zero, 0