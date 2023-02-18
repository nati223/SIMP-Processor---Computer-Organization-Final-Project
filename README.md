# ISA-Simulator---Computer-Organization-Final-Project

Here we present you the final project of Computer Structure project held in Tel Aviv University, Fall 2022.

In this project we will develop a simulator for a RISC type processor named SIMP which is like a MIPS processor but far less complex.
This simulator will simulate a SIMP processor and several input/output types such as lights, 7 segment number display, monochrome monitor with a 256X256 resolution and a hard disk.

Our project is tested with 3 assembly programs, described as follows:

a. A program named fibo.asm that stores starting at memory location 0x100 the Fibonacci series. When an overflow occurs, the program stops.

b. A program square.asm that draws a square. Memory location 0x100 holds the location on the screen of the upper left-hand corner of the square. At location 0x101 is the length of the all the squares edges. The interior of the square is white. The area outside the square is black. You must check that the square can be drawn without going out of the boundaries of the monitor. Note that this program comes with an additional python file to show the drawn square.

c. A program named disktest.asm takes as an input 2 sector numbers in the hard disk and sums the first 8 words (word 0 to word 7) in each sector number. At memory location 0x100 is the result of the first sector and at 0x101 is the result of the second sector. At memory location 0x102 is the larger number of the two.

In order to make it easier to program the processor and create the memory image in the input file memin.txt we will write in this project the assembler program. The assembler will be written in the C programming language and will translate the assembler code written in text in assembler format to machine code. One can assume that the input file is valid.
The assembler is executed using the command line as shown below:

asm.exe program.asm memin.txt

The input file program.asm contains the assembly program, the output file file memin.txt contains 
the memory image (as described above) and is input to the simulator.

The simulator simulates the fetch-decode-execute loop. At the beginning of the run the register “PC”=0. At the beginning of every iteration of this loop the next instruction is fetched, decoded and executed according to the instructions’ contents. At the end of the execution the PC is set to the next instruction in sequence (either 1 or two are added in accordance to the type of instruction fetched as described earlier) unless a jump is executed and the PC is set to the target of the jump instruction. The run concludes when the instruction HALT is executed.
The simulator is written in the C programming language and will be run from a command line application that receives 13 command line parameters as written in the following execution line:

sim.exe memin.txt diskin.txt irq2in.txt memout.txt regout.txt trace.txt hwregtrace.txt cycles.txt leds.txt display7seg.txt diskout.txt monitor.txt monitor.yuv
