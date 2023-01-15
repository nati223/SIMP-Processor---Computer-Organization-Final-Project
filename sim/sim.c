#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

//Struct for commands fetched from memin.txt
typedef struct Instruction {
	char inst_line[6];         //contains the line as String, 5 hexa digits + '\0'
	int opcode;
	int rd;
	int rs;
	int rt;
	int imm;
}Instruction;

////////////////////////////////////////////////////////////////
//Global variables:
#define SIZE 4096
#define DISK_SIZE 16384	//128 sectors * 128 lines
#define MONITOR_SIZE 65536 // 256*256 pixels
#define INT20_MAX 524287
#define INT20_MIN -524288
char ram_arr[SIZE + 1][6]; //array of based on memin.txt
static int pc = 0;
static int instruction_arr_size;
int inst_reg_arr[16]; //array of the registers that are used in instruction execution
char io_registers[23][9] = {"00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000"}; // IO registers
char hard_disk_arr[DISK_SIZE + 1][6]; //array of diskout, each line is 5 hexas + '\0' in the end
static int irq = 0;
int irq_ready = 1;
static int in_irq1 = 0;
static int irq1_op = 0;
static int mem_buffer_address;
static int sector_pos;
static int irq1_cycle_counter = 0;
static unsigned int irq2_interrupt_cycles[SIZE] = { 0 }; // array of all the pc which has irq2in interrupt
static int num_of_irq2_interrupts;
static int irq2_arr_cur_position = 0;
char monitor_arr[MONITOR_SIZE + 1][9];

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

// Auxiliary Functions //

//Slice input string from start index up to and including end index
char * SliceStr(char str[], int start, int end);

//Maps hexa digits to their decimal value and returns it as an integer.
int HexCharToInt(char h);

//Converts an integer to an 8 digit hexa string
void IntToHex8Signed(int dec_num, char hex_num[9]);

//Converts an unsigned integer to an 8 digit hexa string
void IntToHex8Unsigned(unsigned int dec_num, char hex_num[9]);

// Converts a string holds an hexadecimal representation of a number, and returns the number as an integer.
int HexToInt2sComp(char * h);

// Converts a string holds an hexadecimal representation of a number, and returns the number as an unsigned integer.
int HexToIntUnsigned(char * h);

// Initialization Functions //

//Fill hard_disk_arr, based on diskin.txt file
void FillDiskoutArr(FILE * pdiskin);

//Fills instruction_arr, based on memin.txt file
void FillInstArr(FILE * pmemin);

//Fills irq2_interrupt_cycles array, based on irq2in.txt file
void FillIrq2inArr(FILE * pirq2in);

//Fills monitor_arr with with MONITOR_SIZE lines initialized "00000000".
void FillMonitorArr();

//Groups all initiliazation functions under one umbrella, in order to execute them all together when the program starts
void SetArrays(FILE * pdiskin, FILE * pmemin, FILE * pirq2in);

// Main Runtime Functions //

//The function propagates the simulation. It iterates over inst_arr according to the pc, creats a Command struct and sends it to execution. After each instruction, the functions also checks for interrupts and start their execution process if needed.
void FetchInst(FILE * ptrace, FILE * pcycles, FILE * pmemout, FILE * pregout, FILE * pleds, FILE * pdiskout, FILE * pdisplay, FILE * phwregtrace, FILE * pmonitor);

// Takes the 5 hexa digits that are given in memin.txt, and creates a Command struct object with it's desired fields.
void CreateInstruction(char * instruction_line, Instruction * inst);

// Checks in we are in or should interrupt
void CheckIrqStatus();

//This function controls the instruction execution in the program. It chooses the correct instruction according to opcode, and performs the desired operations according to the specifications. Handling clock propagation, and the keeping the correct pc value is also done here. Checking overflow is done for addition, subtraction and multiplication.
void ExecuteInst(Instruction * inst, FILE * ptrace, FILE * pcycles, FILE * pmemout, FILE * pregout, FILE * pleds, FILE * pdiskout, FILE * pdisplay, FILE * phwregtrace, FILE * pmonitor);

//Propagates clock by one each time it's called, and sets it to 0 (cyclicity) after when overflow occurs.
void PropagateClock();

//Checks if overflow has occured in one of the following operations: addition, subtraction, multiplication.
int CheckForOverflow(int opcode, int rs, int rt);

//Writes to trace.txt file according to the given format in the assignemnt.
void TraceWrite(Instruction * inst, FILE * ptrace);

// The routine takes action when irq1 interrupt is recieved. It starts counting clock cycles up to and when reaching 1024, performs reading\writing operations to or from the hard disk.
void HardDiskRoutine();

// Updates the pixel values that are represented in monitor_arr.
void UpdateMonitorArr();

// Writes to hwregtrace.txt after each read or write to a hardware register (after in\out instructions), according to the format specified in the assignment.
void HwRegTraceWrite(FILE * phwregtrace, int rw, int reg_num);

//Writes to leds.txt according to the format specified in the assignment.
void LedsWrite(FILE * pleds);

//Checks if the timer was activated. If it does, it incerements it's value each clock cycle.
void TimerRoutine();

//Writes to display7seg.txt, a file that contains the 7-segment display at every clock cycle when the display changes.
void DisplayWrite(FILE * pdisplay7seg);

// End Of simulation functions //

// Writes the registers content to regout.txt
void RegOutWrite(FILE * pregout);

//Writes the memory content, i.e instruction_arr, to memout.txt
void MemOutWrite(FILE * pmemout);

//Writes the hard disk content to diskout.txt
void DiskOutWrite(FILE * pdiskout);

//Writes the monitor content to monitor.txt
void MonitorWrite(FILE * pmonitor);

//Groups all the procedures above that need to write to the files only at the end of the program. After that, it closes all files that were still open and exits the simulation.
void EndProcedure(FILE * ptrace, FILE * pcycles, FILE * pmemout, FILE * pregout, FILE * pleds, FILE * pdiskout, FILE * pdisplay, FILE * phwregtrace, FILE * pmonitor);


//////////////////////////////////////////////////////////////
// Functions implementation
/////////////////////////////////////////////////////////////

char * SliceStr(char str[], int start, int end)
{
	static char temp[9];
	int i = 0;
	while (start <= end)
	{
		temp[i] = str[start];
		i++;
		start++;
	}
	temp[i] = '\0';

	return temp;
}

int HexCharToInt(char h) {
    if (h >= '0' && h <= '9') {
        return h - '0';
    }
    else if (h >= 'A' && h <= 'F') {
        return h - 'A' + 10;
    }
    else if (h >= 'a' && h <= 'f') {
        return h - 'a' + 10;
    }
    return -1;
}

void IntToHex8Signed(int dec_num, char hex_num[9])
{
	if (dec_num < 0) //if dec_num is negative, add 2^32 to it
		dec_num = dec_num + 4294967296; // dec_num = dec_num + 2^32
	sprintf(hex_num, "%08X", dec_num); //set hex_num to be dec_num in signed hex
}

void IntToHex8Unsigned(unsigned int dec_num, char hex_num[9])
{
	sprintf(hex_num, "%08X", dec_num); //set hex_num to be dec_num in signed hex
}

int HexToInt2sComp(char * h) {
	int i;
	int res = 0;
	int len = strlen(h);
	for (i = 0; i < len; i++)
	{
		res += HexCharToInt(h[len - 1 - i]) * (1 << (4 * i)); // change char by char from right to left, and shift it left 4*i times (2^4i) 
	}
	if ((len < 8) && (res & (1 << (len * 4 - 1)))) // if len is less than 8 and the msb is 1, we want to sign extend the number
	{
		res |= -1 * (1 << (len * 4)); // perform bitwise or with the mask of 8-len times 1's and len time 0's
	}
	return res;
}

int HexToIntUnsigned(char * h) {
	int i;
	unsigned int res = 0;
	int len = strlen(h);
	for (i = 0; i < len; i++)
		res += HexCharToInt(h[len - 1 - i]) * (1 << (4 * i)); // change char by char from right to left, and shift it left 4*i times (2^4i) 
	return res;
}

////////////////////////////////////////////////////
// Setup Functions //
////////////////////////////////////////////////////

void FillIrq2inArr(FILE * irq2in)
{
	//for (int k = 0; k < SIZE; k++)
	//	irq2_interrupt_cycles[k] = -1;
	char line[10]; //because 2^32 is 10 digit long and it's the clock maximum value
	int i = 0;
	while (!feof(irq2in))
	{
		fgets(line, 10, irq2in); 
		irq2_interrupt_cycles[i] = (unsigned int)strtoul(line, NULL, 10);
		i++;
	}
	num_of_irq2_interrupts = i;
	fclose(irq2in);
}

void FillInstArr(FILE * pmemin)
{
	char line[6];
	int i = 0;
	while (!feof(pmemin))
	{
		fscanf(pmemin, "%5[^\n]\n", line); //scans the first 5 chars in a line
		strcpy(ram_arr[i], line); //fills array[i]
		i++;
	}
	instruction_arr_size = i;
	while (i < SIZE)
	{
		strcpy(ram_arr[i], "00000"); // padding with zeros all the empty memory fills array[i]
		i++;
	}
	fclose(pmemin);
}

void FillDiskoutArr(FILE * pdiskin)
{
	char line[6];
	int i = 0;
	while (!feof(pdiskin))
	{
		fscanf(pdiskin, "%5[^\n]\n", line); //scans the first 5 chars in a line
		strcpy(hard_disk_arr[i], line); //disk_out_array[i]
		i++;
	}
	while (i < DISK_SIZE)
	{
		strcpy(hard_disk_arr[i], "00000"); // paddin with zeros all the empty memory disk_out_array[i]
		i++;
	}
	fclose(pdiskin);
}

void SetArrays(FILE * pdiskin, FILE * pmemin, FILE * pirq2in)
{
	FillDiskoutArr(pdiskin); 
	FillIrq2inArr(pirq2in);
	FillInstArr(pmemin);
	FillMonitorArr();
}

/////////////////////////////////////////////////////////////////
// Main process functions
////////////////////////////////////////////////////////////////

void HardDiskRoutine()
{
	if (in_irq1)
	{
		if (irq1_cycle_counter < 1024)
			irq1_cycle_counter++;
		else // Data being read or written will be available in RAM or hard disk only after 1024 cycles.
		{
			int i = 0;
			switch (irq1_op)
			{
			case 1: //read sector
				in_irq1 = 1; // raise that we are in irq1 interrupt
				for (i = sector_pos; i < sector_pos + 128; i++)
				{
					strcpy(ram_arr[mem_buffer_address], hard_disk_arr[i]); // read from hard disk to buffer
					if (mem_buffer_address + 1 == SIZE)
						mem_buffer_address = 0; 
					else
						mem_buffer_address++;
				}
			break;
			case 2: //write command
				in_irq1 = 1; // raise that we are in irq1 interrupt
				for (i = sector_pos; i < sector_pos + 128; i++)
				{
					strcpy(hard_disk_arr[i], ram_arr[mem_buffer_address]); // write to hard_disk from buffer
					if (mem_buffer_address + 1 == SIZE)
						mem_buffer_address = 0;
					else
						mem_buffer_address++;
				}
			break;
			}
			irq1_cycle_counter = 0;
			in_irq1 = 0;
			irq1_op = 0;
			IntToHex8Signed(0, io_registers[14]);  //diskcmd is now 0
			IntToHex8Signed(0, io_registers[17]);  //disk is free now
			IntToHex8Signed(1, io_registers[4]);	// ready to get into irq1 again
		}
	}
}

void TraceWrite(Instruction  * inst, FILE * ptrace)
{
	char pc_hexa[4]; // PC in hexa digits
	char inst_hexa_line[6];
	char r0[9];
	char r1[9];
	char r2[9];
	char r3[9];
	char r4[9];
	char r5[9];
	char r6[9];
	char r7[9];
	char r8[9];
	char r9[9];
	char r10[9];
	char r11[9];
	char r12[9];
	char r13[9];
	char r14[9];
	char r15[9];

	strcpy(inst_hexa_line, inst->inst_line);

	char hexa_num[9];

	IntToHex8Signed(pc, hexa_num);
	strcpy(pc_hexa, SliceStr(hexa_num,5,7));

	IntToHex8Signed(inst_reg_arr[0], hexa_num);
	strcpy(r0, hexa_num);
	
	if((inst->rt == 1) || (inst->rs == 1) || (inst->rd == 1))
		IntToHex8Signed(inst->imm, hexa_num);
	else
		IntToHex8Signed(0,hexa_num);
	strcpy(r1, hexa_num);

	IntToHex8Signed(inst_reg_arr[2], hexa_num);
	strcpy(r2, hexa_num);

	IntToHex8Signed(inst_reg_arr[3], hexa_num);
	strcpy(r3, hexa_num);

	IntToHex8Signed(inst_reg_arr[4], hexa_num);
	strcpy(r4, hexa_num);

	IntToHex8Signed(inst_reg_arr[5], hexa_num);
	strcpy(r5, hexa_num);

	IntToHex8Signed(inst_reg_arr[6], hexa_num);
	strcpy(r6, hexa_num);

	IntToHex8Signed(inst_reg_arr[7], hexa_num);
	strcpy(r7, hexa_num);

	IntToHex8Signed(inst_reg_arr[8], hexa_num);
	strcpy(r8, hexa_num);

	IntToHex8Signed(inst_reg_arr[9], hexa_num);
	strcpy(r9, hexa_num);

	IntToHex8Signed(inst_reg_arr[10], hexa_num);
	strcpy(r10, hexa_num);

	IntToHex8Signed(inst_reg_arr[11], hexa_num);
	strcpy(r11, hexa_num);

	IntToHex8Signed(inst_reg_arr[12], hexa_num);
	strcpy(r12, hexa_num);

	IntToHex8Signed(inst_reg_arr[13], hexa_num);
	strcpy(r13, hexa_num);

	IntToHex8Signed(inst_reg_arr[14], hexa_num);
	strcpy(r14, hexa_num);

	IntToHex8Signed(inst_reg_arr[15], hexa_num);
	strcpy(r15, hexa_num);

	fprintf(ptrace, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n", pc_hexa, inst_hexa_line, r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15);
}

void TimerRoutine()
{
	if (HexToInt2sComp(io_registers[11]) == 1) // Timerenable is 1 indicates that timer request has occured. 0 will stop the count.
	{
		if (HexToIntUnsigned(io_registers[12]) + 1 <= HexToIntUnsigned(io_registers[13])) //if timercurrent is less than timermax
			IntToHex8Unsigned((HexToIntUnsigned(io_registers[12]) + 1), io_registers[12]);

		if (((HexToIntUnsigned(io_registers[12]) + 1) > HexToIntUnsigned(io_registers[13])) && ((HexToIntUnsigned(io_registers[13])) != 0))
		{
			IntToHex8Unsigned(1, io_registers[3]); //if timecurrent is equal to timermax then irq0status is 1
			IntToHex8Unsigned(0, io_registers[12]); //when timecurrent is equal to timermax, we set it to 0 and start counting again
		}
	}
}

void CheckIrqStatus()
{
	if (HexToInt2sComp(io_registers[1]) && HexToInt2sComp(io_registers[4]) && irq_ready) 
		in_irq1 = 1; // we get irq1 interupt
	
	if (((irq2_interrupt_cycles[irq2_arr_cur_position] <= HexToIntUnsigned(io_registers[8]) - 1) && irq_ready) && (irq2_interrupt_cycles[irq2_arr_cur_position] != -1)) // Means we got to a cycle where irq2 is triggered
	{
		if(irq2_arr_cur_position < num_of_irq2_interrupts)
		{
			//printf("got irq2 interrupt at %d\n", irq2_interrupt_pc[irq2_current_index]);
			IntToHex8Signed(1, io_registers[5]); // this if triggerd the irq2status to 1 when there is intruppt
			irq2_arr_cur_position++;
		}
	}

	irq = ((HexToInt2sComp(io_registers[0]) && HexToInt2sComp(io_registers[3])) || ((HexToInt2sComp(io_registers[1])) && HexToInt2sComp(io_registers[4])) || (HexToInt2sComp(io_registers[2]) && HexToInt2sComp(io_registers[5]))) ? 1 : 0;
	IntToHex8Signed(0, io_registers[5]);
}

//this function write to hwregtrace.txt file
void HwRegTraceWrite(FILE * phwregtrace, int rw, int reg_num)
{
	char name[50] = "";
	switch (reg_num) {
	case 0:
		strcpy(name, "irq0enable");
		break;

	case 1:
		strcpy(name, "irq1enable");
		break;

	case 2:
		strcpy(name, "irq2enable");
		break;

	case 3:
		strcpy(name, "irq0status");
		break;

	case 4:
		strcpy(name, "irq1status");
		break;

	case 5:
		strcpy(name, "irq2status");
		break;

	case 6:
		strcpy(name, "irqhandler");
		break;

	case 7:
		strcpy(name, "irqreturn");
		break;

	case 8:
		strcpy(name, "clks");
		break;

	case 9:
		strcpy(name, "leds");
		break;

	case 10:
		strcpy(name, "display7seg");
		break;

	case 11:
		strcpy(name, "timerenable");
		break;

	case 12:
		strcpy(name, "timercurrent");
		break;

	case 13:
		strcpy(name, "timermax");
		break;

	case 14:
		strcpy(name, "diskcmd");
		break;

	case 15:
		strcpy(name, "disksector");
		break;

	case 16:
		strcpy(name, "diskbuffer");
		break;

	case 17:
		strcpy(name, "diskstatus");
		break;
	
	case 18:
		strcpy(name, "reserved");
		break;

	case 19:
		strcpy(name, "reserved");
		break;

	case 20:
		strcpy(name, "monitoraddr");
		break;

	case 21:
		strcpy(name, "monitordata");
		break;

	case 22:
		strcpy(name, "monitorcmd");
		break;
	}

	if (rw) //in command
	{
		fprintf(phwregtrace, "%u READ %s %s\n", HexToIntUnsigned(io_registers[8]), name, io_registers[reg_num]);
	}
	else
	{
		fprintf(phwregtrace, "%u WRITE %s %s\n", HexToIntUnsigned(io_registers[8]), name, io_registers[reg_num]);
	}
}

void CreateInstruction(char * command_line, Instruction * inst)
{
	strcpy(inst->inst_line, command_line);

	inst->opcode = (int)strtol((char[]) {command_line[0], command_line[1], 0}, NULL, 16);
	inst->rd = (int)strtol((char[]) {command_line[2], 0 }, NULL, 16);
	inst->rs = (int)strtol((char[]) {command_line[3], 0 }, NULL, 16);
	inst->rt = (int)strtol((char[]) {command_line[4], 0 }, NULL, 16);
	if((inst->rs == 1) || (inst->rt == 1) || (inst->rd)) // In case we got a command that uses an imm value
	{
		char *immediate = ram_arr[pc+1]; // Point to the next word in file that holds the imm value
		inst->imm = (int)strtol(immediate, NULL, 16);
		if(inst->imm > 524287) // In case we got immediate > 2^19-1, sign bit is on and we need to handle a negative number
			inst->imm -= 1048576;
	}
}

void LedsWrite(FILE * pleds)
{
	fprintf(pleds, "%u %s\n", HexToIntUnsigned(io_registers[8]), io_registers[9]);
}

void DisplayWrite(FILE * pdisplay7seg)
{
	fprintf(pdisplay7seg, "%u %s\n", HexToIntUnsigned(io_registers[8]), io_registers[10]);
}

void FillMonitorArr()
{
	for(int i=0; i< 65536; i++)
	{
		strcpy(monitor_arr[i], "00000000");
	}
}

void UpdateMonitorArr()
{
	int pixel_addr = HexToInt2sComp(io_registers[20]);
	strcpy(monitor_arr[pixel_addr], io_registers[21]);
}

void PropagateClock()
{
	if (HexToIntUnsigned(io_registers[8]) == HexToIntUnsigned("ffffffff")) // Ensuring the clock is cyclic
		IntToHex8Unsigned(0, io_registers[8]);
	else
		IntToHex8Unsigned((HexToIntUnsigned(io_registers[8]) + 1), io_registers[8]);
	
	TimerRoutine();
	HardDiskRoutine();
}

void ExecuteInst(Instruction * inst, FILE * ptrace, FILE * pcycles, FILE * pmemout, FILE * pregout, FILE * pleds, FILE * pdiskout, FILE * pdisplay7seg, FILE * phwregtrace, FILE * pmonitor)
{
	TraceWrite(inst, ptrace); //write to trace before the command
	char hex_num[9];
	char hex_num_temp[9] = "00000000";
	int *result;
	if(inst->rd == 1 || inst->rs == 1 || inst->rt == 1) // $imm is in use
	{
		PropagateClock();
		pc++; // immediate instructions take advance 2 in pc
		if (inst->rd == 1)
			inst_reg_arr[inst->rd] = inst->imm;
		if (inst->rs == 1)
			inst_reg_arr[inst->rs] = inst->imm;
		if (inst->rt == 1)
			inst_reg_arr[inst->rt] = inst->imm;
	}
	switch (inst->opcode) { //cases for the opcode according to the assignment
	case 0: //add
		if (inst->rd != 0 && inst->rd != 1)
		{
			if(CheckForOverflow(inst->opcode, inst_reg_arr[inst->rs], inst_reg_arr[inst->rt]))
				EndProcedure(ptrace, pcycles, pmemout, pregout, pleds, pdiskout, pdisplay7seg, phwregtrace, pmonitor);
			else
				inst_reg_arr[inst->rd] = inst_reg_arr[inst->rs] + inst_reg_arr[inst->rt];
		}
		pc++;
		break;
	case 1: //sub
		if (inst->rd != 0 && inst->rd != 1)
		{
			if(CheckForOverflow(inst->opcode, inst->rs, inst->rt))
				EndProcedure(ptrace, pcycles, pmemout, pregout, pleds, pdiskout, pdisplay7seg, phwregtrace, pmonitor);
			else
				inst_reg_arr[inst->rd] = inst_reg_arr[inst->rs] - inst_reg_arr[inst->rt];
		}
		pc++;
		break;
	case 2: //mul
		if (inst->rd != 0 && inst->rd != 1)
		{
			if(CheckForOverflow(inst->opcode, inst->rs, inst->rt))
				EndProcedure(ptrace, pcycles, pmemout, pregout, pleds, pdiskout, pdisplay7seg, phwregtrace, pmonitor);
			else
				inst_reg_arr[inst->rd] = inst_reg_arr[inst->rs] * inst_reg_arr[inst->rt];
		}
		pc++;
		break;
	case 3: //and
		if (inst->rd != 0 && inst->rd != 1)
			inst_reg_arr[inst->rd] = inst_reg_arr[inst->rs] & inst_reg_arr[inst->rt];
		pc++;
		break;
	case 4: //or
		if (inst->rd != 0 && inst->rd != 1)
			inst_reg_arr[inst->rd] = inst_reg_arr[inst->rs] | inst_reg_arr[inst->rt];
		pc++;
		break;
	case 5: //xor
		if (inst->rd != 0 && inst->rd != 1)
			inst_reg_arr[inst->rd] = inst_reg_arr[inst->rs] ^ inst_reg_arr[inst->rt];
		pc++;
		break;	
	case 6://sll
		if (inst->rd != 0 && inst->rd != 1)
			inst_reg_arr[inst->rd] = inst_reg_arr[inst->rs] << inst_reg_arr[inst->rt];
		pc++;
		break;
	case 7: //sra
		if (inst->rd != 0 && inst->rd != 1)
			inst_reg_arr[inst->rd] = inst_reg_arr[inst->rs] >> inst_reg_arr[inst->rt]; // Shift is arithmetic by default for signed int
		pc++;
		break;
	case 8: //srl
		if (inst->rd != 0 && inst->rd != 1)
			inst_reg_arr[inst->rd] = (inst_reg_arr[inst->rs] >> inst_reg_arr[inst->rt]) & 0x7fffffff; // force MSB to be 0
		pc++;
		break;
	case 9: //beq
		if (inst_reg_arr[inst->rs] == inst_reg_arr[inst->rt])
			pc = inst_reg_arr[inst->rd];
		else
			pc++;
		break;
	case 10: //bne
		if (inst_reg_arr[inst->rs] != inst_reg_arr[inst->rt])
			pc = inst_reg_arr[inst->rd];
		else
			pc++;
		break;
	case 11: //blt
		if (inst_reg_arr[inst->rs] < inst_reg_arr[inst->rt])
			pc = inst_reg_arr[inst->rd];
		else
			pc++;
		break;
	case 12: //bgt
		if (inst_reg_arr[inst->rs] > inst_reg_arr[inst->rt])
			pc = inst_reg_arr[inst->rd];
		else
			pc++;
		break;
	case 13: //ble
		if (inst_reg_arr[inst->rs] <= inst_reg_arr[inst->rt])
			pc = inst_reg_arr[inst->rd];
		else
			pc++;
		break;
	case 14: //bge
		if (inst_reg_arr[inst->rs] >= inst_reg_arr[inst->rt])
			pc = inst_reg_arr[inst->rd];
		else
			pc++;
		break;
	case 15: //jal
		inst_reg_arr[inst->rd] = (pc + 1);
		pc = inst_reg_arr[inst->rs];
		break;
	case 16: //lw
		PropagateClock();
		if (inst->rd != 0 && inst->rd != 1)
			inst_reg_arr[inst->rd] = HexToInt2sComp(ram_arr[(inst_reg_arr[inst->rs]) + inst_reg_arr[inst->rt]]); //FIXME - should look into the function
		pc++;
		break;
	case 17: //sw
		PropagateClock();
		IntToHex8Signed(inst_reg_arr[inst->rd], hex_num_temp);
		char *store;
		store = SliceStr(hex_num_temp,3,8);
		strcpy(ram_arr[(inst_reg_arr[inst->rs] + inst_reg_arr[inst->rt])], store); // store back in memory
		pc++;
		break;
	case 18: //reti
		pc = HexToInt2sComp(io_registers[7]);
		irq_ready = 1;
		break;
	case 19: //in
		inst_reg_arr[inst->rd] = HexToInt2sComp(io_registers[inst_reg_arr[inst->rs] + inst_reg_arr[inst->rt]]);
		pc++;
		HwRegTraceWrite(phwregtrace, 1, inst_reg_arr[inst->rs] + inst_reg_arr[inst->rt]);
		break;
	case 20: //out
		IntToHex8Signed(inst_reg_arr[inst->rd], io_registers[inst_reg_arr[inst->rs] + inst_reg_arr[inst->rt]]);
		HwRegTraceWrite(phwregtrace, 0, inst_reg_arr[inst->rs] + inst_reg_arr[inst->rt]);
		switch (inst_reg_arr[inst->rs] + inst_reg_arr[inst->rt]) 
		{
		case 9: //leds
			LedsWrite(pleds);
			break;
		case 10: //display
			DisplayWrite(pdisplay7seg);
			break;
		case 14: // hard disk
			if((inst_reg_arr[inst->rd] == 1 || inst_reg_arr[inst->rd] == 2) && !(HexToInt2sComp(io_registers[17]))) // read and disk is free
			{
				in_irq1 = 1;
				sector_pos = HexToInt2sComp(io_registers[15]) * 128; // Get the sector starting position in hard_disk_arr
				mem_buffer_address = HexToInt2sComp(io_registers[16]); // Get the starting position of the buffer in memory to read\write from\to.
				IntToHex8Signed(1, io_registers[17]);  //Assign disk status to 1 (busy)
				if(inst_reg_arr[inst->rd] == 1)
					irq1_op = 1;
				else
					irq1_op = 2;
			}
			break;
		case 22: // monitor
			if(HexToInt2sComp(io_registers[22]) == 1)
				UpdateMonitorArr();
			break;
		}
		pc++;
		break;
	case 21: //halt - write files and close them
		PropagateClock(); // after the execute of the command we update the clk
		EndProcedure(ptrace, pcycles, pmemout, pregout, pleds, pdiskout, pdisplay7seg, phwregtrace, pmonitor);
		break;
	}

	PropagateClock(); // after the execute of the command we update the clk
}

void FetchInst(FILE * ptrace, FILE * pcycles, FILE * pmemout, FILE * pregout, FILE * pleds, FILE * pdiskout, FILE * pdisplay7seg, FILE * phwregtrace, FILE * pmonitor)
{
	Instruction new_inst = { NULL, NULL, NULL, NULL, NULL, NULL };
	while (pc < instruction_arr_size) //perform commands until halt or until end of file
	{
		CreateInstruction(ram_arr[pc], &new_inst); //takes the command according to the pc
		ExecuteInst(&new_inst, ptrace, pcycles, pmemout, pregout, pleds, pdiskout, pdisplay7seg, phwregtrace, pmonitor); //perform the command
		CheckIrqStatus();
		if (irq && irq_ready)
		{
			IntToHex8Signed(pc, io_registers[7]);
			pc = HexToInt2sComp(io_registers[6]); //the proccessor is ready to jump to interrupt
			irq_ready = 0;
		}
	}
}

int CheckForOverflow(int opcode, int rs, int rt)
{
	switch(opcode)
	{
		case 0:
		if((rs > 0) && (rt > 0) && (rs > INT20_MAX - rt))
			return 1;
		else if ((rs < 0) && (rt < 0) && (rs < INT20_MIN - rt))
			return 1;
		else
			return 0;
		break;
		case 1:
		if((rs > 0) && (rt < 0) && (rs > INT20_MAX + rt))
			return 1;
		else if((rs < 0) && (rt > 0) && (rs < INT20_MIN + rt))
			return 1;
		else
			return 0;
		break;
		case 2:
		if((rs > 0) && (rt > 0) && (rs > INT20_MAX/rt))
			return 1;
		else if((rs > 0) && (rt < 0) && (rt < INT20_MAX/rs))
			return 1;
		else if((rs < 0) && (rt > 0) && (rs < INT20_MIN/rt))
			return 1;
		else if((rs < 0) && (rt < 0) && (rs < INT20_MAX/rt))
			return 1;
		else
			return 0;
		break;
	}
	return 0;
}

///////////////////////////////////////////////////////
// End of program functions
//////////////////////////////////////////////////////

void RegOutWrite(FILE *pregout)
{
	int i = 0;
	for (i = 2; i < 16; i++) //go over reg_arr and print it to regout
		fprintf(pregout, "%08X\n", inst_reg_arr[i]);
}

void MemOutWrite(FILE *pmemout)
{
	int i = 0;
	for (i = 0; i <= SIZE; i++) //go over instruction_arr and write it to memout
		fprintf(pmemout, "%s\n", ram_arr[i]);
}

void DiskOutWrite(FILE *diskout)
{
	int i = 0;
	for (i = 0; i <= DISK_SIZE; i++) //go over instruction_arr and write it to memout
		fprintf(diskout, "%s\n", hard_disk_arr[i]);
}

void MonitorWrite(FILE * pmonitor)
{
	for(int i=0;i<MONITOR_SIZE;i++) // Go over monitor_arr and write the 2 rightmost hexa digits.
	{
		fprintf(pmonitor, "%s\n", SliceStr(monitor_arr[i],6,8));	
	}
}

void EndProcedure(FILE * ptrace, FILE * pcycles, FILE * pmemout, FILE * pregout, FILE * pleds, FILE * pdiskout, FILE * pdisplay7seg, FILE * phwregtrace, FILE * pmonitor)
{
	fprintf(pcycles, "%u", HexToIntUnsigned(io_registers[8]));
	DiskOutWrite(pdiskout);
	RegOutWrite(pregout);
	MemOutWrite(pmemout);
	MonitorWrite(pmonitor);
	fclose(pmemout);
	fclose(pregout);
	fclose(ptrace);
	fclose(phwregtrace);
	fclose(pleds);
	fclose(pdisplay7seg);
	fclose(pdiskout);
	fclose(pcycles);
	fclose(pmonitor);
	exit(0);
}

int main(int argc, char *argv[])
{
	FILE *memin = fopen(argv[1], "r"), *diskin = fopen(argv[2], "r"), *irq2in = fopen(argv[3], "r"), *memout = fopen(argv[4], "w"), *regout = fopen(argv[5], "w"), *trace = fopen(argv[6], "w"), *hwregtrace = fopen(argv[7], "w"), *cycles = fopen(argv[8], "w"), *leds = fopen(argv[9], "w"), *display7seg = fopen(argv[10], "w"), *diskout = fopen(argv[11], "w"), *monitor = fopen(argv[12], "w");
	if (memin == NULL || irq2in == NULL || diskin == NULL || memout == NULL || regout == NULL || trace == NULL || cycles == NULL || hwregtrace == NULL || leds == NULL || display7seg == NULL || diskout == NULL || monitor == NULL)
	{
		printf("One of the files could not be opened \n ");
		exit(1);
	}
	//Initialize
	SetArrays(diskin, memin, irq2in);
	// Start main process
	FetchInst(trace, cycles, memout, regout, leds, diskout, display7seg, hwregtrace, monitor);
	// If the program did not end earlier, end it now
	EndProcedure(trace, cycles, memout, regout, leds, diskout, display7seg, hwregtrace, monitor);
	return 0;
}