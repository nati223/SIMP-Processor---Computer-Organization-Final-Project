#define _CRT_SECURE_NO_WARNINGS
/////declaretion of the relevant libraries/////
#include <stdio.h> // for IO
#include <math.h> // for arithmetics
#include <string.h> // for string function
#include <stdlib.h> // for strtol

/////declaretion of the constant parameters/////
#define MAX_ROW_LEN 300 // the max line size of the input file
#define MAX_LABEL_LEN 50 // the max lable size
#define MEM_SIZE 4096 // max num of rows in the main memory
#define LEN_OF_ROW 20 // len of each row, didnt use it


/////structs declaration/////

// defened a struct that hold label information
typedef struct Label
{
	char name[MAX_LABEL_LEN + 1];
	int address; // index of output line
}Label;

// defined a struct that hold line information
typedef struct LINE
{
	int opcode;
	int rd;
	int rs;
	int rt;
	int imm;

}LINE;

// defined a struct that hold data for the .word instruction
typedef struct Word
{
	int address;
	int data;
}Word;


/////functions declaration/////
//----initialazing----//
void initilaize_memory_array(int* memory);
int initilaize_label_array(FILE* asm_program, Label* label_array);

//----extract line data----//
void get_label(char* line, Label* label);
int translate_line_to_fields(char* line, LINE* assembly_line, Label* label_array, Word* word, int max_label_array_index);
int get_opcode(char** line);
int get_reg(char** line);
int get_imm(char* line, Label* label_array, int max_label_array_index);
void extract_imm_by_space(char* line);
void translate_to_fields(FILE* program, LINE* assembly_lines, Label* label_array, Word* word, int max_label_array_index, int* memory);
Word* word_to_fields(char* line, Word* word);
void data_to_word(char* data, Word* word);
void address_to_word(char* address, Word* word);


//check information from line----//
int type_of_line(char* line);
int has_label(char* line);
int has_instruction(char* line);
int find_type(char* line);


//----extract spases, hashtags etc----//
char* skip_seperator(char* line);
void erase_hashtag(char* line);
char* erase_spaces_in_the_end(char* line);


//----writing function----//
int hex_value(LINE* assembly_lines);
void write_output_to_file(FILE* output, int* memory);
void close_files(FILE* asm_program, FILE* memin);
void free_allocate_memorry(Label* label_array,LINE* assembly_lines, int* memory);


//----base number exchange----//
void address_hexe_to_int(char* address, int address_int);
void base10_to_base16(char* base10, char* base16);
char int_base16_to_char_base16(int base16);
void char_16base_to_char_16base_16scomp(char* base16);
int char_base16_to_int_base10(char d);
int base10_char_to_base16_int_16scomp(char* base16, char* base10);


///// main /////
int main(int argc, char* argv[]) {
	FILE* asm_program;
	FILE* memin;
	asm_program = fopen(argv[1], "r"); // open in order to read
	memin = fopen(argv[2], "w"); // open to write
	if (asm_program == NULL || memin == NULL) { // // check if fopen failed
		printf("File could not be open\n"); // if it failed, print an error
		return 0;
	}
	Label* label_array = (Label*)calloc(MEM_SIZE, sizeof(Label));
	Word word = { 0,0 };
	int max_label_array_index = initilaize_label_array(asm_program, label_array);
	LINE* assembly_lines = (LINE*)calloc(MEM_SIZE, sizeof(LINE));
	int* memory = (int*) calloc(MEM_SIZE, sizeof(int));
	initilaize_memory_array(memory);
	translate_to_fields(asm_program, assembly_lines, label_array, &word, max_label_array_index, memory);
	write_output_to_file(memin, memory);
	close_files(asm_program, memin);
	free_allocate_memorry(label_array, assembly_lines, memory);
	//_CrtDumpMemoryLeaks();
	return 0;
}


//// initilaize memory array to 00000
void initilaize_memory_array(int* memory)
{
	for (int i = 0; i < MEM_SIZE; i++)
	{
		memory[i] = 0x00000;
	}
}

int type_of_line(char* line) // 0 for empty row, 1 for only label, 2 for label+Rtype, 3 for label + Itype, 4 for only Rtype, 5 for only Itype, 6 for .word
{
	if (1 == has_label(line)) {
		// the line has a label
		// check if the line has instructions as well 
		if (has_instruction(line) == 1) { // has label and instruction
			// check the type of instructions 
			if (find_type(line) == 1) {
				return 2; // has label and Rtype
			}
			return 3; // has label and Itype
		}
		else { // there isnt instruction in addition to the label in the line
			return 1; // only

		}
	}
	else { // the line has no label
		// check if the line has instructions
		if (has_instruction(line) == 1) { // has instruction
			// check the type of instructions 
			if (find_type(line) == 1) // only Rtype
				return 4;
			else
				return 5; // only Itype
		}
		else { // the line has no instructions
			// check if the line has .word instraction
			if (strstr(line, ".word") != NULL) {
				return 6; // has .word instruction
			}

		}
		return 0; // empty row
	}
}

//the function gets a line and check if the line has label. return 1 if true and 0 if false
int has_label(char* line)
{
	if (strchr(line, ':') != NULL) {
		return 1; // there is a label
	}
	else {
		return 0; // there isnt a label
	}
}

// the function gets a line and return 1 if has instruction - Rtype or Itype and 0 if not
int has_instruction(char* line)
{
	if (strchr(line, '$') != NULL) {
		return 1; // there is an instruction Rtype or Itype
	}
	else // there is no instruction
		return 0;
}

//the function gets a pointer to a line and return the type of instruction - Rtype (return 1) or Itype (return 2)
int find_type(char* line)
{
	if (strstr(line, "$imm") != NULL) {
		return 2; // Itype instruction. written by 2 rows
	}
	else
		return 1; // Rtype instruction. written by 1 row
}

int translate_line_to_fields(char* line, LINE* assembly_line, Label* label_array, Word* word, int max_label_array_index)
{
	int int_line_type = type_of_line(line);
	if (int_line_type == 0 || int_line_type == 1)
	{
		return 0; // the line has no instructions to translate to fields
	}
	else if (int_line_type == 6)
	{
		line = skip_seperator(line);
		word_to_fields(line, word);
		return 3;
	}
	else if (int_line_type == 2 || int_line_type == 4) // has instraction Rtype
	{
		line = skip_seperator(line);
		// All the get functions also advances the line pointer
		assembly_line->opcode = get_opcode(&line);
		line = skip_seperator(line);
		assembly_line->rd = get_reg(&line);
		line = skip_seperator(line);
		assembly_line->rs = get_reg(&line);
		line = skip_seperator(line);
		assembly_line->rt = get_reg(&line);
		line = skip_seperator(line);
		return 1;
	}
	else // has Itype instruction
	{
		line = skip_seperator(line);
		// All the get functions also advances the line pointer
		assembly_line->opcode = get_opcode(&line);
		line = skip_seperator(line);
		assembly_line->rd = get_reg(&line);
		line = skip_seperator(line);
		assembly_line->rs = get_reg(&line);
		line = skip_seperator(line);
		assembly_line->rt = get_reg(&line);
		line = skip_seperator(line);
		line = erase_spaces_in_the_end(line);
		assembly_line->imm = get_imm(line, label_array, max_label_array_index);
		return 2;
	}

}

// extract opcode from the line, convet to int in base 16 and update the pointer to the end of the opcode
int get_opcode(char** line)
{
	char cmd2[3]; // for 2 letters opcode
	char cmd3[4]; // dor 3 letter opcode
	char cmd4[5]; // for 4 letters opcode 
	strncpy(cmd2, *line, 2);
	cmd2[2] = '\0';
	strncpy(cmd3, *line, 3);
	cmd3[3] = '\0';
	strncpy(cmd4, *line, 4);
	cmd4[4] = '\0';
	if (strcmp(cmd3, "add") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the register
		return 0x00;
	}
	if (strcmp(cmd3, "sub") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the register
		return 0x01;
	}
	if (strcmp(cmd3, "mul") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the register
		return 0x02;
	}
	if (strcmp(cmd3, "and") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the register
		return 0x03;
	}
	if (strcmp(cmd2, "or") == 0) {
		*line = *line + 2; // update the pointer of the line to point the next text => the register
		return 0x04;
	}
	if (strcmp(cmd3, "xor") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the register
		return 0x05;
	}
	if (strcmp(cmd3, "sll") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the register
		return 0x06;
	}
	if (strcmp(cmd3, "sra") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the register
		return 0x07;
	}
	if (strcmp(cmd3, "srl") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the register
		return 0x08;
	}
	if (strcmp(cmd3, "beq") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the register
		return 0x09;
	}
	if (strcmp(cmd3, "bne") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the register
		return 0x0A;
	}
	if (strcmp(cmd3, "blt") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the register
		return 0x0B;
	}
	if (strcmp(cmd3, "bgt") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the register
		return 0x0C;
	}
	if (strcmp(cmd3, "ble") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the register
		return 0x0D;
	}
	if (strcmp(cmd3, "bge") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the register
		return 0x0E;
	}
	if (strcmp(cmd3, "jal") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the register
		return 0x0F;
	}
	if (strcmp(cmd2, "lw") == 0) {
		*line = *line + 2; // update the pointer of the line to point the next text => the register
		return 0x10;
	}
	if (strcmp(cmd2, "sw") == 0) {
		*line = *line + 2; // update the pointer of the line to point the next text => the register
		return 0x11;
	}
	if (strcmp(cmd4, "reti") == 0) {
		*line = *line + 4; // update the pointer of the line to point the next text => the register
		return 0x12;
	}
	if (strcmp(cmd2, "in") == 0) {
		*line = *line + 2; // update the pointer of the line to point the next text => the register
		return 0x13;
	}
	if (strcmp(cmd3, "out") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the register
		return 0x14;
	}
	if (strcmp(cmd4, "halt") == 0) {
		*line = *line + 4; // update the pointer of the line to point the next text => the register
		return 0x15;
	}
	return -1;
}


//extract registers rd,rs,rt from the line, convet to int in base 16 and update the pointer to the end of the register
int get_reg(char** line)
{
	char cmd3[4]; // for 3 letters reg name
	char cmd4[5]; // dor 4 letter reg name
	char cmd5[6]; // for 5 letters reg name
	strncpy(cmd3, *line, 3);
	cmd3[3] = '\0';
	strncpy(cmd4, *line, 4);
	cmd4[4] = '\0';
	strncpy(cmd5, *line, 5);
	cmd5[5] = '\0';
	if (strcmp(cmd5, "$zero") == 0) {
		*line = *line + 5; // update the pointer of the line to point the next text => the next register / imm
		return 0x0;
	}
	if (strcmp(cmd4, "$imm") == 0) {
		*line = *line + 4; // update the pointer of the line to point the next text => the next register / imm
		return 0x1;
	}
	if (strcmp(cmd3, "$v0") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the next register / imm
		return 0x2;
	}
	if (strcmp(cmd3, "$a0") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the next register / imm
		return 0x3;
	}
	if (strcmp(cmd3, "$a1") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the next register / imm
		return 0x4;
	}
	if (strcmp(cmd3, "$a2") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the next register / imm
		return 0x5;
	}
	if (strcmp(cmd3, "$a3") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the next register / imm
		return 0x6;
	}
	if (strcmp(cmd3, "$t0") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the next register / imm
		return 0x7;
	}
	if (strcmp(cmd3, "$t1") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the next register / imm
		return 0x8;
	}
	if (strcmp(cmd3, "$t2") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the next register / imm
		return 0x9;
	}
	if (strcmp(cmd3, "$s0") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the next register / imm
		return 0xA;
	}
	if (strcmp(cmd3, "$s1") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the next register / imm
		return 0xB;
	}
	if (strcmp(cmd3, "$s2") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the next register / imm
		return 0xC;
	}
	if (strcmp(cmd3, "$gp") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the next register / imm
		return 0xD;
	}
	if (strcmp(cmd3, "$sp") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the next register / imm
		return 0xE;
	}
	if (strcmp(cmd3, "$ra") == 0) {
		*line = *line + 3; // update the pointer of the line to point the next text => the next register / imm
		return 0xF;
	}
	return -1;
}

int get_imm(char* line, Label* label_array, int max_label_array_index)
{
	if ((line[0] == '0') && (line[1] == 'x' || line[1] == 'X')) // written in hexe
	{
		//line[8] = '\0'; // end the line in the end of the imm
		return (int)strtol(line, NULL, 16); // return the hex string as a hex int
	}
	else // is label or 10 base
	{
		int imm_int = 16777216;
		for (int i = 0; i < max_label_array_index; i++) // loop on all the labels
		{
			if (strcmp(line, label_array[i].name) == 0) //find the label
			{
				imm_int = label_array[i].address; // return its address in 10 base
				break;
			}
		}
		if (imm_int == 16777216) // imm not a label
		{
			if (line[0] == '-')
			{
				line = line + 1; // erase the minus
				char base16[] = "00000";
				return base10_char_to_base16_int_16scomp(base16, line);
			}
			// else it is a positive int
			return (int)strtol(line, NULL, 10); // return its address in 10 base int
		}
		else // imm is a label	
			return imm_int; // it doesnt metter what base the int
	}
}


void extract_imm_by_space(char* line)
{
	int i = 0;
	while (line[i] != '\0') // run until the end of the string
	{
		if (line[i] == ' ' || line[i] == '\t' || line[i] == '\n')
			break;
		i++;
	}
	line[i] = '\0'; // end line in the first space/tab
}

// erase spaces and "," from the beggining of line, return a new pointer to the begining of the line
char* skip_seperator(char* line)
{
	int i = 0;
	while ((line[i] != '\0') && (line[i] == ' ' || line[i] == ',' || line[i] == '\t' || line[i] == '\n')) // run until the end of the string or the first letter
	{
		i++; // count the spaces and "," until the begining of the string
	}
	line = line + i;
	return line;
}

void translate_to_fields(FILE* program, LINE* assembly_lines, Label* label_array, Word* word, int max_label_array_index, int* memory)
{
	char line[MAX_ROW_LEN];
	int line_index = 0;
	rewind(program); // reset the pointer of asm_program file to the begining of the file																			
	while (NULL != fgets(line, MAX_ROW_LEN, program))
	{
		erase_hashtag(line); // erase the # and note if exists
		int output = translate_line_to_fields(line, &(assembly_lines[line_index]), label_array, word, max_label_array_index);
		if (output == 1 || output == 2) // Rtype or Itype
		{
			memory[line_index] = hex_value(&(assembly_lines[line_index]));
			line_index++;
			if (output == 2) // Itype
			{
				memory[line_index] = assembly_lines[line_index - 1].imm;
				line_index++;
			}
		}
		else if (output == 3) // .word instruction
			memory[word->address] = word->data;
	}
}

// function that combain all the fields to one 5 letters hex value 
int hex_value(LINE* assembly_lines)
{
	return (4096 * (assembly_lines->opcode) + 256 * (assembly_lines->rd) + 16 * (assembly_lines->rs) + assembly_lines->rt);
}

// extract the label from the line. update name field of label
void get_label(char* line, Label* label)
{
	// check the type of line and if the line has no label print error
	int int_line_type = type_of_line(line); // 0 for empty row, 1 for only label, 2 for label+Rtype, 3 for label + Itype, 4 for only Rtype, 5 for only Itype, 6 for .word
	if (int_line_type == 0 || int_line_type == 4 || int_line_type == 5 || int_line_type == 6) { // line has no label
		printf("error line hasn't a label but sent to get_label function\n"); // if it failed, print an error
	}
	else // line has label
	{
		int i;
		for (i = 0; i < MAX_ROW_LEN; i++) // run on the line until the first ":"
		{
			if (line[i] == ':')
				break;
		}
		line[i] = '\0'; // end the string on the ":"
		line = skip_seperator(line);
		strcpy(label->name, line); // update the name field in label as the string of the label without spaces
	}
}


//create an array of all the labels in the file, contains name and address in the output file
int initilaize_label_array(FILE* asm_program, Label* label_array)
{
	// initilaze parameters
	int PC = 0; // hold the output address
	int label_array_index = 0; // hold the index of the label, counting one by one from the beggining
	char line[MAX_ROW_LEN]; // hold the content of the line
	while (NULL != fgets(line, MAX_ROW_LEN, asm_program))
	{// keep reading the file line by line until the end of the file
		erase_hashtag(line); // erase the # and note if exists
		int int_line_type;
		// check the type of the line
		int_line_type = type_of_line(line); // 0 for empty row, 1 for only label, 2 for label+Rtype, 3 for label + Itype, 4 for only Rtype, 5 for only Itype, 6 for .word
		if (int_line_type == 1 || int_line_type == 2 || int_line_type == 3) { // has label
			Label* label = &(label_array[label_array_index]);
			get_label(line, label); // puts the label from the line to the veriable label in the name field => label.name = "name of label"
			label_array[label_array_index].address = PC; // put the address of the label in the output file into the address field of the label
			label_array_index++; // raise the label_array_index for the next label
			// update PC
		}
		if (int_line_type == 2 || int_line_type == 4) // has Rtype instruction 
			PC++; // takes only one line in the output file
		if (int_line_type == 3 || int_line_type == 5) // has Itype instruction
			PC = PC + 2; // takes 2 lines in the output file
		else // has .word instruction or empty line
			continue;
	}
	return label_array_index;
}



void write_output_to_file(FILE* output, int* memory)
{
	for (int i = 0; i < MEM_SIZE - 1; i++)
	{
		fprintf(output, "%05x\n", memory[i]);
	}
	fprintf(output, "%05x", memory[MEM_SIZE - 1]);
}


Word* word_to_fields(char* line, Word* word)
{
	line = strtok(line, "#"); // erase the # and note if exists
	char* temp = strtok(line, ".word"); // erase the .word
	char* current_line_pointer = line + (strlen(temp)) - 1; // pointer to the current place in the line, initialaze as the start of the line without .word
	char* end_line_pointer = line + (strlen(temp)) + strlen(line); // pointer to the end of the line without the #
	char* address;
	char* data;
	address = strtok(current_line_pointer, " \t\n"); // extract the address without spaces
	address_to_word(address, word);
	current_line_pointer += strlen(current_line_pointer) + 1; // update the pointer of the line to ignore the opcod
	data = strtok(current_line_pointer, " \t\n"); // extract the data without spaces
	data_to_word(data, word);
	return word;
}

void address_to_word(char* address, Word* word)
{
	if ((address[0] == '0') && (address[1] == 'x' || address[1] == 'X')) // address is in hexe
	{
		int int_address = 0;
		address_hexe_to_int(address, int_address);
		word->address = int_address;
	}
	else // address is in 10 base number
	{

		word->address = (int)strtol(address, NULL, 10); // convert the num in the string to int

	}
}

void address_hexe_to_int(char* address, int address_int)
{
	int i = 0;
	while (address[i] != '\0')
	{
		int d = 0;
		if (address[i] >= '0' && address[i] <= '9')
			d = address[i] - '0';
		else if (address[i] >= 'A' && address[i] <= 'F')
			d = address[i] - 'A' + 10;
		else if (address[i] >= 'a' && address[i] <= 'f')
			d = address[i] - 'a' + 10;
		address_int = address_int * 16 + d;
		i++;
	}
}


void data_to_word(char* data, Word* word)
{
	if ((data[0] == '0') && (data[1] == 'x' || data[1] == 'X')) // data is in hexe
	{
		word->data = (int)strtol(data, NULL, 16);
		(data);
	}
	else // data is in 10 base
	{
		word->data = (int)strtol(data, NULL, 10);

	}
}


void erase_hashtag(char* line)
{
	int i = 0;
	while ((line[i] != '\0') && (line[i] != '\n')) // run until the end of the string or the first letter
	{
		if (line[i] == '#')
		{
			line[i] = '\0';
			break;
		}
		i++; // count the index in the string
	}
}

char* erase_spaces_in_the_end(char* line)
{
	int i = 0;
	while ((line[i] != '\0') && (line[i] != '\n')) // run until the end of the string or the first letter
	{
		if (line[i] == '\t' || line[i] == ' ')
		{
			break;
		}
		i++; // count the index in the string
	}
	line[i] = '\0';
	return line;
}

void base10_to_base16(char* base10, char* base16)
{
	// convert the base10 string to an int 
	int base10_int = strtol(base10, NULL, 10);
	// initialize a base16 string
	// convert the base10 int to base 16 by dividing by 16 and storing the reminder in base16 string
	int j = 4;
	char str[] = "0123456789abcdef";
	do
	{
		// accessing the charecter from the string using the value of base10_int %16
		int num = base10_int % 16;
		base16[j] = str[num];
		base10_int = base10_int / 16;
		j--;
	} while (base10_int > 0);
}

char int_base16_to_char_base16(int base16)
{
	if (base16 == 15)
		return 'f';
	if (base16 == 14)
		return 'e';
	if (base16 == 13)
		return 'd';
	if (base16 == 12)
		return 'c';
	if (base16 == 11)
		return 'b';
	if (base16 == 10)
		return 'a';
	if (base16 == 9)
		return '9';
	if (base16 == 9)
		return '8';
	if (base16 == 7)
		return '7';
	if (base16 == 6)
		return '6';
	if (base16 == 5)
		return '5';
	if (base16 == 4)
		return '4';
	if (base16 == 3)
		return '3';
	if (base16 == 2)
		return '2';
	if (base16 == 1)
		return '1';
	if (base16 == 0)
		return '0';
	else
		return '-'; // error
}

void char_16base_to_char_16base_16scomp(char* base16)
{
	// (fffff - base16) +1
	for (int i = 0; i < 5; i++)
	{
		int digit = char_base16_to_int_base10(base16[i]);
		base16[i] = int_base16_to_char_base16(15 - digit);
	}
	if (base16[4] == 'f')
	{
		base16[4] = '0';
		if (base16[3] != 'f')
			base16[3] = (char)((int)base16[3] + 1);
		else
		{
			base16[3] = '0';
			if (base16[2] != 'f')
				base16[2] = (char)((int)base16[2] + 1);
			else
			{
				base16[2] = '0';
				if (base16[1] != 'f')
					base16[1] = (char)((int)base16[1] + 1);
				else
				{
					base16[1] = '0';
					base16[0] = (char)((int)base16[0] + 1);
				}
			}
		}
	}
	else
		base16[4] = (char)((int)base16[4] + 1);
}

int char_base16_to_int_base10(char d)
{
	if (d == 'f')
		return 15;
	if (d == 'e')
		return 14;
	if (d == 'd')
		return 13;
	if (d == 'c')
		return 12;
	if (d == 'b')
		return 11;
	if (d == 'a')
		return 10;
	else
		return (int)d - '0';
}
int base10_char_to_base16_int_16scomp(char* base16, char* base10)
{
	base10_to_base16(base10, base16);
	char_16base_to_char_16base_16scomp(base16);
	return strtol(base16, NULL, 16);
}

void close_files(FILE* asm_program, FILE* memin)
{
	fclose(asm_program);
	fclose(memin);
}

void free_allocate_memorry(Label* label_array, LINE* assembly_lines, int* memory)
{
	free(label_array);
	free(assembly_lines);
	free(memory);
}