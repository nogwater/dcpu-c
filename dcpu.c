#include <stdio.h>
#include <stdlib.h>


// http://0x10c.com/doc/dcpu-16.txt
// NOTE: I'm going to represent basic opcodes in a char as 0000oooo
// 			and non-basic as 10oooooo

typedef struct s_dcpu
{
	// Registers
	unsigned short A, B, C, X, Y, Z, I, J;
	// Overflow
	unsigned short O;
	// Program Counter
	unsigned short PC;
	// Stack Pointer
	unsigned short SP;
	// Memory
	unsigned short RAM[0x10000];
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
		, cpu->J
		, cpu->O
		, cpu->PC
		, cpu->SP);
}

void dcpu_print_video(dcpu_t *cpu)
{
	int base = 0x8000;
	char row = 0;
	char col = 0; 
	for (row = 0; row < 12; row++) {
		for (col = 0; col < 32; col++) {
			printf("%c", cpu->RAM[base + (row * 32 + col)]);
		}
		printf("\n");
	}
}

unsigned short dcpu_inst_make(char op, char a, char b)
{
	if ((op & 0x80) == 0) {
		// basic bbbbbbaaaaaaoooo
		return ((b & 0x3f) << 10) | ((a & 0x3f) << 4) | (op & 0xf);
	} else {
		// non-basic aaaaaaoooooo0000
		return ((a & 0x3f) << 10) | ((op & 0x3f) << 4) | 0;
	}
}

void dcpu_inst_parse(unsigned short word, unsigned char *op, unsigned char *a, unsigned char *b)
{
	// printf("parsing %x\n", word);
	*op = word & 0x0f;
	// printf("op %x\n", *op);
	if (*op != 0) { // basic
		// bbbbbbaaaaaaoooo
		*a  = ((word >> 4) & 0x3f);
		*b  = (word >> 10);
	} else { // non-basic
		// aaaaaaoooooo0000
		// printf("looks like op is non-basic\n");
		*op = ((word >> 4) & 0x3f) | 0x80; //set the high-bit to indicate non-basic
		*a = (word >> 10);
		*b = 0;
	}
	printf("Parsed %08x as op:%02x a:%02x b:%02x\n", word, *op, *a, *b);
}

unsigned short *dcpu_point_at_value(dcpu_t *cpu, unsigned char val)
{
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

	unsigned short next_word;

	// registers
	if (val == 0x00) return &(cpu->A);
	if (val == 0x01) return &(cpu->B);
	if (val == 0x02) return &(cpu->C);
	if (val == 0x03) return &(cpu->X);
	if (val == 0x04) return &(cpu->Y);
	if (val == 0x05) return &(cpu->Z);
	if (val == 0x06) return &(cpu->I);
	if (val == 0x07) return &(cpu->J);

	// [register]
	if (val == 0x08) return cpu->RAM + cpu->A;
	if (val == 0x09) return cpu->RAM + cpu->B;
	if (val == 0x0a) return cpu->RAM + cpu->C;
	if (val == 0x0b) return cpu->RAM + cpu->X;
	if (val == 0x0c) return cpu->RAM + cpu->Y;
	if (val == 0x0d) return cpu->RAM + cpu->Z;
	if (val == 0x0e) return cpu->RAM + cpu->I;
	if (val == 0x0f) return cpu->RAM + cpu->J;

	// [next word + register]
	if (val >= 0x10 && val <= 0x17) {
		next_word = cpu->RAM[cpu->PC++];
		if (val == 0x10) return cpu->RAM + cpu->A + next_word;
		if (val == 0x11) return cpu->RAM + cpu->B + next_word;
		if (val == 0x12) return cpu->RAM + cpu->C + next_word;
		if (val == 0x13) return cpu->RAM + cpu->X + next_word;
		if (val == 0x14) return cpu->RAM + cpu->Y + next_word;
		if (val == 0x15) return cpu->RAM + cpu->Z + next_word;
		if (val == 0x16) return cpu->RAM + cpu->I + next_word;
		if (val == 0x17) return cpu->RAM + cpu->J + next_word;
	}

	// POP
	if (val == 0x18) return cpu->RAM + cpu->SP++;
	// PEEK
	if (val == 0x19) return cpu->RAM + cpu->SP;
	// PUSH
	if (val == 0x1a) return cpu->RAM + (--(cpu->SP));
	// SP
	if (val == 0x1b) return &(cpu->SP);
	// PC
	if (val == 0x1c) return &(cpu->PC);
	// O
	if (val == 0x1d) return &(cpu->O);
	// [next word]
	if (val == 0x1e) {
		next_word = cpu->RAM[cpu->PC++];
		printf("trying to point at %08x in RAM\n", next_word);
		return cpu->RAM + next_word;
	}
	// can't reference literals
	// next word (literal)
	// if (val == 0x1f) {
	// 	next_word = cpu->RAM[cpu->PC++];
	// 	return next_word;
	// }
	// // literal value 0x00-0x1f (literal)
	// return val - 0x20;
}

unsigned short dcpu_get_value(dcpu_t *cpu, unsigned char val)
{
	// for most references just deref the pointer
	if (val <= 0x1e) {		
		return *dcpu_point_at_value(cpu, val);
	}
	// next word (literal)
	if (val == 0x1f) {
		unsigned short next_word;
		next_word = cpu->RAM[cpu->PC++];
		return next_word;
	}
	// literal value 0x00-0x1f (literal)
	if (val >= 0x20) {
		return val - 0x20;
	}
}

unsigned short dcpu_exec1(dcpu_t *cpu)
{
	// executes the next instruction
	unsigned char op, a, b;
	dcpu_inst_parse(cpu->RAM[cpu->PC++], &op, &a, &b);

	// SET a, b - sets a to b
	if (op == 0x1) {
		*(dcpu_point_at_value(cpu, a)) = dcpu_get_value(cpu, b);
	}
}

void print_bits(unsigned short word)
{
	char i = 0;
	for (i = 15; i >= 0; i--) {
		printf("%d", (word >> i) & 1);
	}
	printf("\n");
}

int main(int argc, char *argv)
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
	// unsigned char op, a, b;
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

	// SET C, 0x8000 (stored in next word)
	cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x2, 0x1f);
	cpu->RAM[cpu->PC+1] = 0x8000;
	dcpu_exec1(cpu);
	printf("C: %02x\n", cpu->C);

	// SET [C], 0x0048 (stored in next word)
	cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x0a, 0x1f);
	cpu->RAM[cpu->PC+1] = 0x0048;
	dcpu_exec1(cpu);
	dcpu_print_video(cpu);

	// SET [next word], next word
	cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x1e, 0x1f);
	cpu->RAM[cpu->PC+1] = 0x8001; // next word (address of video RAM satrt)
	cpu->RAM[cpu->PC+2] = 0x0065; // next word ('e')
	dcpu_exec1(cpu);
	dcpu_print_video(cpu);
	// SET [next word], next word
	cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x1e, 0x1f);
	cpu->RAM[cpu->PC+1] = 0x8002; // next word (address of video RAM satrt)
	cpu->RAM[cpu->PC+2] = 0x006c; // next word ('l')
	dcpu_exec1(cpu);
	dcpu_print_video(cpu);
	// SET [next word], [next word]
	cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x1e, 0x1e);
	cpu->RAM[cpu->PC+1] = 0x8003; // next word (address of video RAM satrt)
	cpu->RAM[cpu->PC+2] = 0x8002; // next word [0x8002] -> 'l'
	dcpu_exec1(cpu);
	dcpu_print_video(cpu);

	unsigned short bad = dcpu_point_at_value(cpu, 0x20);
	printf("bad: %04x\n", bad);

	return 0;
}