#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


// http://0x10c.com/doc/dcpu-16.txt
// NOTE: I'm going to represent basic opcodes in an uint8_t as 0000oooo
// 			and non-basic as 10oooooo

#define RAM_SIZE	0x10000
#define STACK_ADDR	0xffff
#define VIDEO_ADDR	0x8000


typedef struct s_dcpu
{
	// Registers
	uint16_t A, B, C, X, Y, Z, I, J;
	// Overflow
	uint16_t O;
	// Program Counter
	uint16_t PC;
	// Stack Pointer
	uint16_t SP;
	// Memory
	uint16_t RAM[RAM_SIZE];
} dcpu_t;


dcpu_t *dcpu_make()
{
	dcpu_t *cpu = malloc(sizeof(dcpu_t));
	int i = 0;
	for (i = 0; i < 0x10000; i++) {
		cpu->RAM[i] = 0;
	}
	cpu->SP = 0xffff;
	return cpu;
}

uint16_t dcpu_inst_make(uint8_t op, uint8_t a, uint8_t b)
{
	if ((op & 0x80) == 0) {
		// basic bbbbbbaaaaaaoooo
		return ((b & 0x3f) << 10) | ((a & 0x3f) << 4) | (op & 0xf);
	} else {
		// non-basic aaaaaaoooooo0000
		return ((a & 0x3f) << 10) | ((op & 0x3f) << 4) | 0;
	}
}

void dcpu_inst_parse(uint16_t word, uint8_t *op, uint8_t *a, uint8_t *b)
{
	*op = word & 0x0f;
	if (*op != 0) { // basic bbbbbbaaaaaaoooo
		*a  = ((word >> 4) & 0x3f);
		*b  = (word >> 10);
	} else { // non-basic  aaaaaaoooooo0000
		*op = ((word >> 4) & 0x3f) | 0x80; //set the high-bit to indicate non-basic
		*a = (word >> 10);
		*b = 0;
	}
}

// true if the reference is for a literal
bool is_literal(uint8_t ref)
{
	// 0x1f: reference to the next word as a literal
	// 0x20-0x3f: this ref is a value (ref - 0x20)
	return (ref >= 0x1f);
}

// returns a pointer to something based on the provided reference
uint16_t *dcpu_ref(dcpu_t *cpu, uint8_t ref)
{
	//    Reference: meaning
	//    0x00-0x07: register (A, B, C, X, Y, Z, I or J, in that order)
	//    0x08-0x0f: [register]
	//    0x10-0x17: [next word + register]
	//         0x18: POP / [SP++]
	//         0x19: PEEK / [SP]
	//         0x1a: PUSH / [--SP]
	//         0x1b: SP
	//         0x1c: PC
	//         0x1d: O
	//         0x1e: [next word]
	//         0x1f: next word (literal)
	//    0x20-0x3f: literal value 0x00-0x1f (literal)

	uint16_t next_word;

	// registers
	if (ref == 0x00) return &(cpu->A);
	if (ref == 0x01) return &(cpu->B);
	if (ref == 0x02) return &(cpu->C);
	if (ref == 0x03) return &(cpu->X);
	if (ref == 0x04) return &(cpu->Y);
	if (ref == 0x05) return &(cpu->Z);
	if (ref == 0x06) return &(cpu->I);
	if (ref == 0x07) return &(cpu->J);

	// [register]
	if (ref == 0x08) return cpu->RAM + cpu->A;
	if (ref == 0x09) return cpu->RAM + cpu->B;
	if (ref == 0x0a) return cpu->RAM + cpu->C;
	if (ref == 0x0b) return cpu->RAM + cpu->X;
	if (ref == 0x0c) return cpu->RAM + cpu->Y;
	if (ref == 0x0d) return cpu->RAM + cpu->Z;
	if (ref == 0x0e) return cpu->RAM + cpu->I;
	if (ref == 0x0f) return cpu->RAM + cpu->J;

	// [next word + register]
	if (ref >= 0x10 && ref <= 0x17) {
		next_word = cpu->RAM[cpu->PC++];
		if (ref == 0x10) return cpu->RAM + cpu->A + next_word;
		if (ref == 0x11) return cpu->RAM + cpu->B + next_word;
		if (ref == 0x12) return cpu->RAM + cpu->C + next_word;
		if (ref == 0x13) return cpu->RAM + cpu->X + next_word;
		if (ref == 0x14) return cpu->RAM + cpu->Y + next_word;
		if (ref == 0x15) return cpu->RAM + cpu->Z + next_word;
		if (ref == 0x16) return cpu->RAM + cpu->I + next_word;
		if (ref == 0x17) return cpu->RAM + cpu->J + next_word;
	}

	// POP
	if (ref == 0x18) return cpu->RAM + cpu->SP++;
	// PEEK
	if (ref == 0x19) return cpu->RAM + cpu->SP;
	// PUSH
	if (ref == 0x1a) return cpu->RAM + (--(cpu->SP));
	// SP
	if (ref == 0x1b) return &(cpu->SP);
	// PC
	if (ref == 0x1c) return &(cpu->PC);
	// O
	if (ref == 0x1d) return &(cpu->O);
	// [next word]
	if (ref == 0x1e) {
		next_word = cpu->RAM[cpu->PC++];
		return cpu->RAM + next_word;
	}
	// can't reference literals
	return NULL;
}

// get the value at the reference
uint16_t dcpu_get(dcpu_t *cpu, uint8_t ref)
{
	// for most references just deref the pointer
	if (ref <= 0x1e) {		
		return *dcpu_ref(cpu, ref);
	}
	// next word (literal)
	if (ref == 0x1f) {
		uint16_t next_word;
		next_word = cpu->RAM[cpu->PC++];
		return next_word;
	}
	// literal value 0x00-0x1f (literal)
	return ref - 0x20; // ref >= 0x20)
}

// executes the next instruction
void dcpu_exec1(dcpu_t *cpu)
{
	uint8_t op, a, b;
	uint16_t *a_ptr;
	uint32_t result; // holds O and the value for math
	dcpu_inst_parse(cpu->RAM[cpu->PC++], &op, &a, &b);

	switch (op) {
		case 0x01:// SET a, b - sets a to b
			a_ptr = dcpu_ref(cpu, a); // a is always handled by the processor before b
			result = dcpu_get(cpu, b);
			// *assignment* fail silently if trying to assign to a literal
			if (is_literal(a) == false) *a_ptr = (uint16_t)result;
			break;
		case 0x02:// ADD a, b - sets a to a+b, sets O to 0x0001 if there is's an overflow, 0x0 othterwise
			a_ptr = dcpu_ref(cpu, a);
			result = *a_ptr + dcpu_get(cpu, b);
			if (is_literal(a) == false) *a_ptr = (uint16_t)result;
			cpu->O = (uint16_t)((result >> 16) & 0xffff);
			break;
		case 0x03:// SUB a, b - sets a to a-b, sets O to 0xffff if there is's an underflow, 0x0 othterwise
			a_ptr = dcpu_ref(cpu, a);
			result = *a_ptr - dcpu_get(cpu, b);
			if (is_literal(a) == false) *a_ptr = (uint16_t)result;
			cpu->O = (uint16_t)((result >> 16) & 0xffff);
			break;
		case 0x04:// MUL a, b - sets a to a*b, sets a to a*b, sets O to ((a*b)>>16)&0xffff
			a_ptr = dcpu_ref(cpu, a);
			result = *a_ptr * dcpu_get(cpu, b);
			if (is_literal(a) == false) *a_ptr = (uint16_t)result;
			cpu->O = (uint16_t)((result >> 16) & 0xffff);
			break;
		case 0x05:// DIV a, b - sets a to a*b, sets a to a*b, sets O to ((a*b)>>16)&0xffff
			a_ptr = dcpu_ref(cpu, a);
			result = *a_ptr - dcpu_get(cpu, b);
			if (is_literal(a) == false) *a_ptr = (uint16_t)result;
			cpu->O = (uint16_t)((result >> 16) & 0xffff);
			break;
		default:
			printf("Unrecognized opcode %02x\n", op);
	}
}

void dcpu_print_state(dcpu_t *cpu)
{
	printf("A:%04x B:%04x C:%04x X:%04x Y:%04x Z:%04x I:%04x J:%04x O:%04x PC:%04x SP:%04x \n"
		, cpu->A
		, cpu->B
		, cpu->C
		, cpu->X
		, cpu->Y
		, cpu->Z
		, cpu->I
		, cpu->J
		, cpu->O
		, cpu->PC
		, cpu->SP);
}

void dcpu_print_video(dcpu_t *cpu)
{
	char row = 0;
	char col = 0; 
	for (row = 0; row < 12; row++) {
		for (col = 0; col < 32; col++) {
			printf("%c", cpu->RAM[VIDEO_ADDR + (row * 32 + col)]);
		}
		printf("\n");
	}
}

void dcpu_print_stack(dcpu_t *cpu)
{
	uint16_t sp = STACK_ADDR;
	printf("Stack: ");
	for (sp = STACK_ADDR - 1; sp >= cpu->SP; sp--) {
		printf("%04x ", cpu->RAM[sp]);
	}
	printf("\n");
}

void print_bits(uint16_t word)
{
	char i = 0;
	for (i = 15; i >= 0; i--) {
		printf("%d", (word >> i) & 1);
	}
	printf("\n");
}

int main(int argc, char *argv[])
{
	dcpu_t *cpu = dcpu_make();

	// cpu->A = 1;
	// int v = 0x8000;
	// cpu->RAM[v++] = 0x48;
	// cpu->RAM[v++] = 0x65;
	// cpu->RAM[v++] = 0x6c;
	// cpu->RAM[v++] = 0x6c;
	// cpu->RAM[v++] = 0x6f;
	// cpu->RAM[v++] = 0x20;
	// cpu->RAM[v++] = 0x57;
	// cpu->RAM[v++] = 0x6f;
	// cpu->RAM[v++] = 0x72;
	// cpu->RAM[v++] = 0x6c;
	// cpu->RAM[v++] = 0x64;
	// cpu->RAM[v++] = 0x21;
	// dcpu_print_state(cpu);
	// dcpu_print_video(cpu);

	// enter a simple program
	// cpu->RAM[0] = dcpu_inst_make(0xf, 0xa, 0xb);
	// uint8_t op, a, b;
	// cpu->RAM[0] = dcpu_inst_make(0x1 | 0x80, 0xa, 0xb);
	// dcpu_inst_parse(cpu->RAM[0], &op, &a, &b);

	// // SET A, 1 (literal)
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x0, 0x20 + 1);
	// printf("A: %02x\n", cpu->A);
	// dcpu_exec1(cpu);
	// printf("A: %02x\n", cpu->A);
	// // SET B, A
	// printf("B: %02x\n", cpu->B);
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x1, 0x0);
	// dcpu_exec1(cpu);
	// printf("B: %02x\n", cpu->B);

	// // SET C, 0x8000 (stored in next word)
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x2, 0x1f);
	// cpu->RAM[cpu->PC+1] = 0x8000;
	// dcpu_exec1(cpu);
	// printf("C: %02x\n", cpu->C);

	// // SET [C], 0x0048 (stored in next word)
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x0a, 0x1f);
	// cpu->RAM[cpu->PC+1] = 0x0048;
	// dcpu_exec1(cpu);
	// dcpu_print_video(cpu);

	// // SET [next word], next word
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x1e, 0x1f);
	// cpu->RAM[cpu->PC+1] = 0x8001; // next word (address of video RAM satrt)
	// cpu->RAM[cpu->PC+2] = 0x0065; // next word ('e')
	// dcpu_exec1(cpu);
	// dcpu_print_video(cpu);
	// // SET [next word], next word
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x1e, 0x1f);
	// cpu->RAM[cpu->PC+1] = 0x8002; // next word (address of video RAM satrt)
	// cpu->RAM[cpu->PC+2] = 0x006c; // next word ('l')
	// dcpu_exec1(cpu);
	// dcpu_print_video(cpu);
	// // SET [next word], [next word]
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x1e, 0x1e);
	// cpu->RAM[cpu->PC+1] = 0x8003; // next word (address of video RAM satrt)
	// cpu->RAM[cpu->PC+2] = 0x8002; // next word [0x8002] -> 'l'
	// dcpu_exec1(cpu);
	// dcpu_print_video(cpu);

	// test assigning to a literal
	// SET 0, A
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x20, 0x0);

	// uint16_t *bad = dcpu_point_at_value(cpu, 0x20);
	// printf("bad: %p\n", bad);

	// // ADD A, 2
	// cpu->A = 1;
	// dcpu_print_state(cpu);
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x2, 0x0, 0x22);
	// dcpu_exec1(cpu);
	// dcpu_print_state(cpu);

	// // stack test
	// // 3 + (2 + 4)
	// dcpu_print_state(cpu);
	// dcpu_print_stack(cpu);
	// // SET PUSH, 3
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x1a, 0x23);
	// dcpu_exec1(cpu);
	// dcpu_print_state(cpu);
	// dcpu_print_stack(cpu);
	// // SET PUSH, 2
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x1a, 0x22);
	// dcpu_exec1(cpu);
	// dcpu_print_state(cpu);
	// dcpu_print_stack(cpu);
	// // SET PUSH, 4
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x1a, 0x24);
	// dcpu_exec1(cpu);
	// dcpu_print_state(cpu);
	// dcpu_print_stack(cpu);
	// // SET A, POP
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x0, 0x18);
	// dcpu_exec1(cpu);
	// dcpu_print_state(cpu);
	// dcpu_print_stack(cpu);
	// // ADD A, POP
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x2, 0x0, 0x18);
	// dcpu_exec1(cpu);
	// dcpu_print_state(cpu);
	// dcpu_print_stack(cpu);
	// // ADD A, POP
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x2, 0x0, 0x18);
	// dcpu_exec1(cpu);
	// dcpu_print_state(cpu);
	// dcpu_print_stack(cpu);

	// // overflow
	// // SET A, 2
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x0, 0x22);
	// dcpu_exec1(cpu);
	// dcpu_print_state(cpu);
	// // ADD A, 0xfff (in next word)
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x2, 0x0, 0x1f);
	// cpu->RAM[cpu->PC+1] = 0xffff;
	// dcpu_exec1(cpu);
	// dcpu_print_state(cpu);

	// underflow
	// SET A, 1
	cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x0, 0x21);
	dcpu_exec1(cpu);
	dcpu_print_state(cpu);
	// ADD A, 2
	cpu->RAM[cpu->PC] = dcpu_inst_make(0x3, 0x0, 0x22);
	dcpu_exec1(cpu);
	dcpu_print_state(cpu);



	return 0;
}