#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

//Struct for commands fetched from memin.txt
typedef struct Command {
	char inst[6];//contains the line as String, 5 hexa digits + '\0'
	int opcode;
	int rd;
	int rs;
	int rt;
	int imm;
}Command;

////////////////////////////////////////////////////////////////
//Global variables:
#define SIZE 4096
#define DISK_SIZE 16384        //128 sectors * 128 lines
#define MONITOR_SIZE 65536      // 256*256 pixels
char instruction_arr[SIZE + 1][6]; //array of based on memin.txt
static int pc = 0;
static int instruction_arr_size;
int inst_reg_arr[16]; //array of the registers that are used in instruction execution
char io_registers[23][9] = {"00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000"}; // IO registers
char hard_disk_arr[DISK_SIZE + 1][6]; //array of diskout, each line is 5 hexas + '\0' in the end
static int irq = 0;
int irq_ready = 1;
static int in_irq1 = 0;
static int irq1_cycle_counter = 0;
int irq2_interrupt_cycles[SIZE] = { 0 }; // array of all the pc which has irq2in interrupt
static int irq2_arr_cur_position = 0;
int reti_waiting = 0; //this flag indicates us if we are in the middle of an interrupt
char monitor_arr[MONITOR_SIZE + 1][9];

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

// Auxiliary Functions //

char * SliceStr(char str[], int start, int end);

int HexCharToInt(char h);

void IntToHex8(int dec_num, char hex_num[9]);

int HexToInt2sComp(char * h);

// Initialization Functions //

void FillDiskoutArr(FILE * pdiskin);

void FillInstArr(FILE * pmemin);

void FillIrq2inArr(FILE * pirq2in);

void FillMonitorArr();

void SetArrays(FILE * pdiskin, FILE * pmemin, FILE * pirq2in);

// Main Runtime Functions //

void FetchInst(FILE * ptrace, FILE * pcycles, FILE * pmemout, FILE * pregout, FILE * pleds, FILE * pdiskout, FILE * pdisplay, FILE * phwregtrace, FILE * pmonitor);

void ExecuteInst(Command * cmd, FILE * ptrace, FILE * pcycles, FILE * pmemout, FILE * pregout, FILE * pleds, FILE * pdiskout, FILE * pdisplay, FILE * phwregtrace, FILE * pmonitor);

void PropagateClock();

int CheckForOverflow(int opcode, int rs, int rt);

void TraceWrite(Command * cmd, FILE * ptrace);

void Count1024();

void HardDiskRoutine(char diskout[][6]);

void UpdateMonitorArr();

void CheckIrqStatus();

void HwRegTraceWrite(FILE * phwregtrace, int rw, int reg_num);

void CreateCommand(char * command_line, Command * cmd);

void LedsWrite(FILE * pleds);

void TimerHandle(int inc_by);

void DisplayWrite(FILE * pdisplay7seg);

// End Of Program Functions //

void RegOutWrite(FILE * pregout);

void MemOutWrite(FILE * pmemout);

void DiskOutWrite(FILE * pdiskout);

void MonitorWrite(FILE * pmonitor);

void EndProcedure(FILE * ptrace, FILE * pcycles, FILE * pmemout, FILE * pregout, FILE * pleds, FILE * pdiskout, FILE * pdisplay, FILE * phwregtrace, FILE * pmonitor);


//////////////////////////////////////////////////////////////
// Functions implementation
/////////////////////////////////////////////////////////////

//Slice input string from start index up to and including end index
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

//Maps hexa digits to their decimal value and returns it as an integer.
int HexCharToInt(char h) {
	short res;
	switch (h) {
	case 'A':
		res = 10;
		break;
	case 'B':
		res = 11;
		break;
	case 'C':
		res = 12;
		break;
	case 'D':
		res = 13;
		break;
	case 'E':
		res = 14;
		break;
	case 'F':
		res = 15;
		break;
	case 'a':
		res = 10;
		break;
	case 'b':
		res = 11;
		break;
	case 'c':
		res = 12;
		break;
	case 'd':
		res = 13;
		break;
	case 'e':
		res = 14;
		break;
	case 'f':
		res = 15;
		break;
	default:
		res = atoi(&h); // if char < 10 change it to int
		break;
	}
	return res;
}

//Converts and integer to a 8 digit hexa string
void IntToHex8(int dec_num, char hex_num[9])
{
	if (dec_num < 0) //if dec_num is negative, add 2^32 to it
		dec_num = dec_num + 4294967296; // dec_num = dec_num + 2^32
	sprintf(hex_num, "%08X", dec_num); //set hex_num to be dec_num in signed hex
}

// Converts a string holds an hexadecimal representation of a number, and returns the number as an integer.
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

////////////////////////////////////////////////////
// Setup Functions //
////////////////////////////////////////////////////


void FillIrq2inArr(FILE * irq2in)
{
	for (int k = 0; k < SIZE; k++)
		irq2_interrupt_cycles[k] = -1;
	char mline[6];
	int i = 0;
	while (!feof(irq2in))
	{
		fgets(mline, 10, irq2in); //scans the first 8 chars in a line - FIXME: should make this line clearer
		irq2_interrupt_cycles[i] = atoi(mline);
		i++;
	}
	fclose(irq2in);
}

//copy memin to array char "instruction_arr" 
void FillInstArr(FILE * pmemin)
{
	char line[6];
	int i = 0;
	while (!feof(pmemin))
	{
		fscanf(pmemin, "%5[^\n]\n", line); //scans the first 5 chars in a line
		strcpy(instruction_arr[i], line); //fills array[i]
		i++;
	}
	instruction_arr_size = i;
	while (i < SIZE)
	{
		strcpy(instruction_arr[i], "00000"); // paddin with zeros all the empty memory fills array[i]
		i++;
	}
	fclose(pmemin);
}

//copy the content of diksin into array disk_out_array
void FillDiskoutArr(FILE * pdiskin)
{
	char mline[6];
	int i = 0;
	while (!feof(pdiskin))
	{
		fscanf(pdiskin, "%5[^\n]\n", mline); //scans the first 5 chars in a line
		strcpy(hard_disk_arr[i], mline); //disk_out_array[i]
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

// if we get disk read/write command, we start counting to 1024 before the next command.
void Count1024()
{
	if (in_irq1)
	{
		if (irq1_cycle_counter < 1024)
			irq1_cycle_counter++;
		else
		{
			irq1_cycle_counter = 0;
			in_irq1 = 0;
			IntToHex8(0, io_registers[14]);  //diskcmd is now 0
			IntToHex8(0, io_registers[17]);  //disk if free now
			IntToHex8(1, io_registers[4]);	//FIXME - irq1 status is 1, and now we know we have finished
		}
	}
}

//this function conatins the logic of the disk handle
void HardDiskRoutine(char diskout[][6])
{
	if (!HexToInt2sComp(io_registers[17])) // The disk is free to read/write
	{
		int k = 0;
		char temp[20];
		int sector = HexToInt2sComp(io_registers[15]) * 128;
		int mem_address = HexToInt2sComp(io_registers[16]);
		//char str[9]; not in use
		switch (HexToInt2sComp(io_registers[14])) {
		case 0: //no command
			break;
		case 1: //read sector
			IntToHex8(1, io_registers[17]);  //Assign disk status to 1
			in_irq1 = 1; // we get irq1 interupt
			for (k = sector; k < sector + 128; k++)
			{
				strcpy(instruction_arr[mem_address], diskout[k]); // read diskin into memin
				if (mem_address + 1 == SIZE)
					mem_address = 0;
				else
					mem_address++;
			}
			break;
		case 2: //write command
			in_irq1 = 1; // we get irq1 interupt
			IntToHex8(1, io_registers[17]);  //Assign disk status to 1
			for (k = sector; k < sector + 128; k++)
			{
				strcpy(diskout[k], instruction_arr[mem_address]);
				if (mem_address + 1 == SIZE)
					mem_address = 0;
				else
					mem_address++;
			}
			break;
		}
	}
}


//write to trace.txt, documents the state of all regs and PC before execution of the instruction in line
void TraceWrite(Command  * com, FILE * ptrace)
{
	char pch[4]; // PC in hexa digits
	char inst[6];
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

	strcpy(inst, com->inst);

	char hex_num[9];

	IntToHex8(pc, hex_num);
	strcpy(pch, SliceStr(hex_num,5,7));

	IntToHex8(inst_reg_arr[0], hex_num);
	strcpy(r0, hex_num);
	
	if((com->rt == 1) || (com->rs == 1) || (com->rd == 1))
		IntToHex8(com->imm, hex_num);
	else
		IntToHex8(0,hex_num);
	strcpy(r1, hex_num);

	IntToHex8(inst_reg_arr[2], hex_num);
	strcpy(r2, hex_num);

	IntToHex8(inst_reg_arr[3], hex_num);
	strcpy(r3, hex_num);

	IntToHex8(inst_reg_arr[4], hex_num);
	strcpy(r4, hex_num);

	IntToHex8(inst_reg_arr[5], hex_num);
	strcpy(r5, hex_num);

	IntToHex8(inst_reg_arr[6], hex_num);
	strcpy(r6, hex_num);

	IntToHex8(inst_reg_arr[7], hex_num);
	strcpy(r7, hex_num);

	IntToHex8(inst_reg_arr[8], hex_num);
	strcpy(r8, hex_num);

	IntToHex8(inst_reg_arr[9], hex_num);
	strcpy(r9, hex_num);

	IntToHex8(inst_reg_arr[10], hex_num);
	strcpy(r10, hex_num);

	IntToHex8(inst_reg_arr[11], hex_num);
	strcpy(r11, hex_num);

	IntToHex8(inst_reg_arr[12], hex_num);
	strcpy(r12, hex_num);

	IntToHex8(inst_reg_arr[13], hex_num);
	strcpy(r13, hex_num);

	IntToHex8(inst_reg_arr[14], hex_num);
	strcpy(r14, hex_num);

	IntToHex8(inst_reg_arr[15], hex_num);
	strcpy(r15, hex_num);

	fprintf(ptrace, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n", pch, inst, r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15);
}

//contain the logic of the timer
void TimerHandle(int inc_by)
{
	if (HexToInt2sComp(io_registers[11]) == 1)
		if (HexToInt2sComp(io_registers[12]) + inc_by <= HexToInt2sComp(io_registers[13])) //if timercurrent is less than timermax
			IntToHex8((HexToInt2sComp(io_registers[12]) + inc_by), io_registers[12]);

	if (((HexToInt2sComp(io_registers[12]) + inc_by) > HexToInt2sComp(io_registers[13])) && ((HexToInt2sComp(io_registers[13])) != 0))
	{
		IntToHex8(1, io_registers[3]); //if timecurrent is equal to timermax then irq0status is 1
		IntToHex8(0 + inc_by - 1, io_registers[12]); //when timecurrent is equal to timermax, we set it to 0 and start counting again
	}
}

// check if there is an interrupt
void CheckIrqStatus()
{
	if (HexToInt2sComp(io_registers[1]) && HexToInt2sComp(io_registers[4]) && irq_ready) 
		in_irq1 = 1; // we get irq1 interupt
	
	if (((irq2_interrupt_cycles[irq2_arr_cur_position] <= HexToInt2sComp(io_registers[8]) - 1) && irq_ready) && (irq2_interrupt_cycles[irq2_arr_cur_position] != -1)) // Means we got to a cycle where irq2 is triggered
	{
		if(irq2_interrupt_cycles[irq2_arr_cur_position] != -1)
		{
			//printf("got irq2 interrupt at %d\n", irq2_interrupt_pc[irq2_current_index]);
			IntToHex8(1, io_registers[5]); // this if triggerd the irq2status to 1 when there is intruppt
			irq2_arr_cur_position++;
		}
	}

	irq = ((HexToInt2sComp(io_registers[0]) && HexToInt2sComp(io_registers[3])) || ((HexToInt2sComp(io_registers[1])) && HexToInt2sComp(io_registers[4])) || (HexToInt2sComp(io_registers[2]) && HexToInt2sComp(io_registers[5]))) ? 1 : 0;
	IntToHex8(0, io_registers[5]);
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
		fprintf(phwregtrace, "%d READ %s %s\n", HexToInt2sComp(io_registers[8]), name, io_registers[reg_num]);
	}
	else
	{
		fprintf(phwregtrace, "%d WRITE %s %s\n", HexToInt2sComp(io_registers[8]), name, io_registers[reg_num]);
	}
}
// gets a command line, and divides it to its components in com
void CreateCommand(char * command_line, Command * com)
{
	strcpy(com->inst, command_line);

	com->opcode = (int)strtol((char[]) {command_line[0], command_line[1], 0}, NULL, 16);
	com->rd = (int)strtol((char[]) {command_line[2], 0 }, NULL, 16);
	com->rs = (int)strtol((char[]) {command_line[3], 0 }, NULL, 16);
	com->rt = (int)strtol((char[]) {command_line[4], 0 }, NULL, 16);
	if((com->rs == 1) || (com->rt == 1) || (com->rd)) // In case we got a command that uses an imm value
	{
		char *immediate = instruction_arr[pc+1]; // Point to the next word in file that holds the imm value
		com->imm = (int)strtol(immediate, NULL, 16);
		if(com->imm > 524287) // In case we got immediate > 2^19-1, sign bit is on and we need to handle a negative number
			com->imm -= 1048576;
	}
}
//this function write to led.txt file
void LedsWrite(FILE * pleds)
{
	fprintf(pleds, "%d %s\n", HexToInt2sComp(io_registers[8]), io_registers[9]);
}
//this function write to display7seg.txt file
void DisplayWrite(FILE * pdisplay7seg)
{
	fprintf(pdisplay7seg, "%d %s\n", HexToInt2sComp(io_registers[8]), io_registers[10]);
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

//increment the clock according to the instruction type
void PropagateClock()
{
	if (HexToInt2sComp(io_registers[8]) == HexToInt2sComp("ffffffff")) // Ensuring the clock is cyclic
		IntToHex8(0, io_registers[8]);
	else
		IntToHex8((HexToInt2sComp(io_registers[8]) + 1), io_registers[8]);
}

//according to the opcode, perform the command, write to trace, manage pc and cycles, and if 'halt' write to files and close them.
// this is the heart of the program, which incharge to execute the command, manage the clock, the timer and the disk
void ExecuteInst(Command * cmd, FILE * ptrace, FILE * pcycles, FILE * pmemout, FILE * pregout, FILE * pleds, FILE * pdiskout, FILE * pdisplay7seg, FILE * phwregtrace, FILE * pmonitor)
{
	// FIXME - didn't go over the constants or function below until switch
	TraceWrite(cmd, ptrace); //write to trace before the command
	//Determine by how much cycles the command took
	int inc_by = 1;
	int dec_num = 0;  
	char hex_num[9];
	char hex_num_temp[9] = "00000000";
	int *result;
	switch (cmd->opcode) { //cases for the opcode according to the assignment
	case 0: //add
		if (cmd->rd != 0 && cmd->rd != 1)
		{
			if(cmd->rs == 1 || cmd->rt == 1) //$imm is in use
			{
				PropagateClock();
				pc++; // immediate instructions advance 2 in pc
				if (cmd->rs == 1)
					inst_reg_arr[cmd->rs] = cmd->imm;
				if (cmd->rt == 1)
					inst_reg_arr[cmd->rt] = cmd->imm;
			}
			if(CheckForOverflow(cmd->rs, cmd->rt, cmd->opcode))
				EndProcedure(ptrace, pcycles, pmemout, pregout, pleds, pdiskout, pdisplay7seg, phwregtrace, pmonitor);
			else
				inst_reg_arr[cmd->rd] = inst_reg_arr[cmd->rs] + inst_reg_arr[cmd->rt];
		}
		pc++;
		break;
	case 1: //sub
		if (cmd->rd != 0 && cmd->rd != 1)
		{
			if(cmd->rs == 1 || cmd->rt == 1) // $imm is in use
			{
				PropagateClock();
				pc++; // immediate instructions advance 2 in pc
				if (cmd->rs == 1)
					inst_reg_arr[cmd->rs] = cmd->imm;
				if (cmd->rt == 1)
					inst_reg_arr[cmd->rt] = cmd->imm;
			}
			if(CheckForOverflow(cmd->rs, cmd->rt, cmd->opcode))
				EndProcedure(ptrace, pcycles, pmemout, pregout, pleds, pdiskout, pdisplay7seg, phwregtrace, pmonitor);
			else
				inst_reg_arr[cmd->rd] = inst_reg_arr[cmd->rs] - inst_reg_arr[cmd->rt];
		}
		pc++;
		break;
	case 2: //mul
		if (cmd->rd != 0 && cmd->rd != 1)
		{
			if(cmd->rs == 1 || cmd->rt == 1) // $imm is in use
			{
				PropagateClock();
				pc++; // immediate instructions advance 2 in pc
				if (cmd->rs == 1)
					inst_reg_arr[cmd->rs] = cmd->imm;
				if (cmd->rt == 1)
					inst_reg_arr[cmd->rt] = cmd->imm;
			}
			if(CheckForOverflow(cmd->rs, cmd->rt, cmd->opcode))
				EndProcedure(ptrace, pcycles, pmemout, pregout, pleds, pdiskout, pdisplay7seg, phwregtrace, pmonitor);
			else
				inst_reg_arr[cmd->rd] = inst_reg_arr[cmd->rs] * inst_reg_arr[cmd->rt];
		}
		pc++;
		break;
	case 3: //and
		if (cmd->rd != 0 && cmd->rd != 1)
		{
			if(cmd->rs == 1 || cmd->rt == 1) // $imm is in use
			{
				PropagateClock();
				pc++; // immediate instructions take advance 2 in pc
				if (cmd->rs == 1)
					inst_reg_arr[cmd->rs] = cmd->imm;
				if (cmd->rt == 1)
					inst_reg_arr[cmd->rt] = cmd->imm;
			}
			inst_reg_arr[cmd->rd] = inst_reg_arr[cmd->rs] & inst_reg_arr[cmd->rt];
		}
		pc++;
		break;
	case 4: //or
		if (cmd->rd != 0 && cmd->rd != 1)
		{
			if(cmd->rs == 1 || cmd->rt == 1) // $imm is in use
			{
				PropagateClock();
				pc++; // immediate instructions take advance 2 in pc
				if (cmd->rs == 1)
					inst_reg_arr[cmd->rs] = cmd->imm;
				if (cmd->rt == 1)
					inst_reg_arr[cmd->rt] = cmd->imm;
			}
			inst_reg_arr[cmd->rd] = inst_reg_arr[cmd->rs] | inst_reg_arr[cmd->rt];
		}
		pc++;
		break;
	case 5: //xor
		if (cmd->rd != 0 && cmd->rd != 1)
		{
			if(cmd->rs == 1 || cmd->rt == 1) // $imm is in use
			{
				PropagateClock();
				pc++; // immediate instructions take advance 2 in pc
				if (cmd->rs == 1)
					inst_reg_arr[cmd->rs] = cmd->imm;
				if (cmd->rt == 1)
					inst_reg_arr[cmd->rt] = cmd->imm;
			}
			inst_reg_arr[cmd->rd] = inst_reg_arr[cmd->rs] ^ inst_reg_arr[cmd->rt];
		}
		pc++;
		break;	
	case 6://sll
		if (cmd->rd != 0 && cmd->rd != 1)
		{
			if(cmd->rs == 1 || cmd->rt == 1) // $imm is in use
			{
				PropagateClock();
				pc++; // immediate instructions take advance 2 in pc
				if (cmd->rs == 1)
					inst_reg_arr[cmd->rs] = cmd->imm;
				if (cmd->rt == 1)
					inst_reg_arr[cmd->rt] = cmd->imm;
			}
			inst_reg_arr[cmd->rd] = inst_reg_arr[cmd->rs] << inst_reg_arr[cmd->rt];
		}
		pc++;
		break;
	case 7: //sra
		if (cmd->rd != 0 && cmd->rd != 1)
		{
			if(cmd->rs == 1 || cmd->rt == 1) // $imm is in use
			{
				PropagateClock();
				pc++; // immediate instructions take advance 2 in pc
				if (cmd->rs == 1)
					inst_reg_arr[cmd->rs] = cmd->imm;
				if (cmd->rt == 1)
					inst_reg_arr[cmd->rt] = cmd->imm;
			}
			inst_reg_arr[cmd->rd] = inst_reg_arr[cmd->rs] >> inst_reg_arr[cmd->rt]; // Shift is arithmetic by default for signed int
		}
		pc++;
		break;
	case 8: //srl
		if (cmd->rd != 0 && cmd->rd != 1)
		{
			if(cmd->rs == 1 || cmd->rt == 1) // $imm is in use
			{
				PropagateClock();
				pc++; // immediate instructions take advance 2 in pc
				if (cmd->rs == 1)
					inst_reg_arr[cmd->rs] = cmd->imm;
				if (cmd->rt == 1)
					inst_reg_arr[cmd->rt] = cmd->imm;
			}
			inst_reg_arr[cmd->rd] = (inst_reg_arr[cmd->rs] >> inst_reg_arr[cmd->rt]) & 0x7fffffff; // force MSB to be 0
		}
		pc++;
		break;
	case 9: //beq
		if (inst_reg_arr[cmd->rs] == inst_reg_arr[cmd->rt])
		{
			if (cmd->rd == 1)
			{
				PropagateClock();
				inst_reg_arr[cmd->rd] = cmd->imm;
			}
			pc = inst_reg_arr[cmd->rd];
		}
		else
			pc++;
		break;
	case 10: //bne
		if (inst_reg_arr[cmd->rs] != inst_reg_arr[cmd->rt])
		{
			if (cmd->rd == 1)
			{
				PropagateClock();
				inst_reg_arr[cmd->rd] = cmd->imm;
			}
			pc = inst_reg_arr[cmd->rd];

		}
		else
			pc++;
		break;
	case 11: //blt
		if (inst_reg_arr[cmd->rs] < inst_reg_arr[cmd->rt])
		{
			if (cmd->rd == 1)
			{
				PropagateClock();
				inst_reg_arr[cmd->rd] = cmd->imm;
			}
			pc = inst_reg_arr[cmd->rd];
		}
		else
			pc++;
		break;
	case 12: //bgt
		if (cmd->rd == 1)
		{
			PropagateClock();
			inst_reg_arr[cmd->rd] = cmd->imm;
			pc++;
		}
		if (inst_reg_arr[cmd->rs] > inst_reg_arr[cmd->rt])
			pc = inst_reg_arr[cmd->rd];
		else
			pc++;
		break;
	case 13: //ble
		if (cmd->rd == 1)
		{
			PropagateClock();
			inst_reg_arr[cmd->rd] = cmd->imm;
			pc++;	
		}
		if (inst_reg_arr[cmd->rs] <= inst_reg_arr[cmd->rt])
			pc = inst_reg_arr[cmd->rd];
		else
			pc++;
		break;
	case 14: //bge
		if (cmd->rd == 1)
		{
			PropagateClock();
			inst_reg_arr[cmd->rd] = cmd->imm;
			pc++;
		}
		if (inst_reg_arr[cmd->rs] >= inst_reg_arr[cmd->rt])
			pc = inst_reg_arr[cmd->rd];
		else
			pc++;
		break;
	case 15: //jal
		if (cmd->rd == 1 || cmd->rs == 1) // $imm is in use
		{
			PropagateClock();
			pc++;
			if(cmd->rd == 1)
				inst_reg_arr[cmd->rd] = cmd->imm;
			if(cmd->rs == 1)
				inst_reg_arr[cmd->rs] = cmd->imm;
		}
		inst_reg_arr[cmd->rd] = (pc + 1);
		pc = inst_reg_arr[cmd->rs];
		break;
	case 16: //lw
		PropagateClock();
		if (cmd->rd != 0 && cmd->rd != 1)
		{
			if(cmd->rs == 1 || cmd->rt == 1) // $imm is in use
			{
				PropagateClock();
				pc++; // immediate instructions take advance 2 in pc
				if (cmd->rs == 1)
					inst_reg_arr[cmd->rs] = cmd->imm;
				if (cmd->rt == 1)
					inst_reg_arr[cmd->rt] = cmd->imm;
			}
			inst_reg_arr[cmd->rd] = HexToInt2sComp(instruction_arr[(inst_reg_arr[cmd->rs]) + inst_reg_arr[cmd->rt]]); //FIXME - should look into the function
		}
		pc++;
		break;
	case 17: //sw
		PropagateClock();
		if(cmd->rd == 1 || cmd->rs == 1 || cmd->rt == 1) // $imm is in use
		{
			PropagateClock();
			pc++; // immediate instructions take advance 2 in pc
			if (cmd->rd == 1)
				inst_reg_arr[cmd->rd] = cmd->imm;
			if (cmd->rs == 1)
				inst_reg_arr[cmd->rs] = cmd->imm;
			if (cmd->rt == 1)
				inst_reg_arr[cmd->rt] = cmd->imm;
		}
		IntToHex8(inst_reg_arr[cmd->rd], hex_num_temp);
		char *store;
		store = SliceStr(hex_num_temp,3,8);
		strcpy(instruction_arr[(inst_reg_arr[cmd->rs] + inst_reg_arr[cmd->rt])], store); // store back in memory
		printf("%s\n", instruction_arr[(inst_reg_arr[cmd->rs] + inst_reg_arr[cmd->rt])]);
		pc++;
		break;
	case 18: //reti
		pc = HexToInt2sComp(io_registers[7]);
		irq_ready = 1;
		break;
	case 19: //in
		if(cmd->rs == 1 || cmd->rt == 1) // $imm is in use
		{
			PropagateClock();
			pc++; // immediate instructions take advance 2 in pc
			if (cmd->rs == 1)
				inst_reg_arr[cmd->rs] = cmd->imm;
			if (cmd->rt == 1)
				inst_reg_arr[cmd->rt] = cmd->imm;
		}
		inst_reg_arr[cmd->rd] = HexToInt2sComp(io_registers[inst_reg_arr[cmd->rs] + inst_reg_arr[cmd->rt]]);
		pc++;
		HwRegTraceWrite(phwregtrace, 1, inst_reg_arr[cmd->rs] + inst_reg_arr[cmd->rt]);
		break;
	case 20: //out
		if(cmd->rd == 1 || cmd->rs == 1 || cmd->rt == 1) // $imm is in use
		{
			PropagateClock();
			pc++; // immediate instructions take advance 2 in pc
			if (cmd->rd == 1)
				inst_reg_arr[cmd->rd] = cmd->imm;
			if (cmd->rs == 1)
				inst_reg_arr[cmd->rs] = cmd->imm;
			if (cmd->rt == 1)
				inst_reg_arr[cmd->rt] = cmd->imm;
		}
		IntToHex8(inst_reg_arr[cmd->rd], io_registers[inst_reg_arr[cmd->rs] + inst_reg_arr[cmd->rt]]);
		HwRegTraceWrite(phwregtrace, 0, inst_reg_arr[cmd->rs] + inst_reg_arr[cmd->rt]);
		switch (inst_reg_arr[cmd->rs] + inst_reg_arr[cmd->rt]) 
		{
		case 9: //leds
			LedsWrite(pleds);
			break;
		case 10: //display
			DisplayWrite(pdisplay7seg);
			break;
		case 14: // diskcmd
			HardDiskRoutine(hard_disk_arr); // FIXME - go over this function
			break;
		case 22:
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

	if (cmd->rd == 1 || cmd->rs == 1 || cmd->rt == 1) // in case of I format inst.
		inc_by++;
	if (cmd->opcode == 16 || cmd->opcode == 17) // in case of use of lw or sw
		inc_by++;
	PropagateClock(); // after the execute of the command we update the clk
	Count1024();
	TimerHandle(inc_by);// evey command we update the timer via the function timer handle
}

// Fetches instruction from instruction_arr and passes it to execution. Checks after each instruction for interrupts.
void FetchInst(FILE * ptrace, FILE * pcycles, FILE * pmemout, FILE * pregout, FILE * pleds, FILE * pdiskout, FILE * pdisplay7seg, FILE * phwregtrace, FILE * pmonitor)
{
	Command new_cmd = { NULL, NULL, NULL, NULL, NULL, NULL };
	while (pc < instruction_arr_size) //perform commands until halt or until end of file
	{
		CreateCommand(instruction_arr[pc], &new_cmd); //takes the command according to the pc
		ExecuteInst(&new_cmd, ptrace, pcycles, pmemout, pregout, pleds, pdiskout, pdisplay7seg, phwregtrace, pmonitor); //perform the command
		CheckIrqStatus();
		if (irq && irq_ready)
		{
			IntToHex8(pc, io_registers[7]);
			pc = HexToInt2sComp(io_registers[6]); //the proccessor is ready to jump to interrupt
			irq_ready = 0;
		}
	}
}

// This function checks for each arithmetic operation that is prone to overflow, if it will occur
int CheckForOverflow(int opcode, int rs, int rt)
{
	switch(opcode)
	{
		case 0:
		if((rs > 0) && (rt > 0) && (rs > INT32_MAX - rt))
			return 1;
		else if ((rs < 0) && (rt < 0) && (rs < INT32_MIN - rt))
			return 1;
		else
			return 0;
		break;
		case 1:
		if((rs > 0) && (rt < 0) && (rs > INT32_MAX + rt))
			return 1;
		else if((rs < 0) && (rt > 0) && (rs < INT32_MIN + rt))
			return 1;
		else
			return 0;
		break;
		case 2:
		if((rs > 0) && (rt > 0) && (rs > INT32_MAX/rt))
			return 1;
		else if((rs > 0) && (rt < 0) && (rt < INT32_MAX/rs))
			return 1;
		else if((rs < 0) && (rt > 0) && (rs < INT32_MIN/rt))
			return 1;
		else if((rs < 0) && (rt < 0) && (rs < INT32_MAX/rt))
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

//write to regout.txt
void RegOutWrite(FILE *pregout)
{
	int i = 0;
	for (i = 2; i < 16; i++) //go over reg_arr and print it to regout
		fprintf(pregout, "%08X\n", inst_reg_arr[i]);
}

//Write to memout.txt
void MemOutWrite(FILE *pmemout)
{
	int i = 0;
	for (i = 0; i <= SIZE; i++) //go over instruction_arr and write it to memout
		fprintf(pmemout, "%s\n", instruction_arr[i]);
}

//Write to diskout.txt
void DiskOutWrite(FILE *diskout)
{
	int i = 0;
	for (i = 0; i <= DISK_SIZE + 1; i++) //go over instruction_arr and write it to memout
		fprintf(diskout, "%s\n", hard_disk_arr[i]);
}

//Write to monitor.txt
void MonitorWrite(FILE * pmonitor)
{
	for(int i=0;i<MONITOR_SIZE;i++) // Go over monitor_arr and write the 2 rightmost hexa digits.
	{
		fprintf(pmonitor, "%s\n", SliceStr(monitor_arr[i],6,8));	
	}
}

void EndProcedure(FILE * ptrace, FILE * pcycles, FILE * pmemout, FILE * pregout, FILE * pleds, FILE * pdiskout, FILE * pdisplay7seg, FILE * phwregtrace, FILE * pmonitor)
{
	fprintf(pcycles, "%d", HexToInt2sComp(io_registers[8]));
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
}