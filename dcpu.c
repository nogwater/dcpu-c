#include "dcpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// http://0x10c.com/doc/dcpu-16.txt
//
// NOTE: I'm going to represent basic opcodes in an uint8_t as 0000oooo
// 			and non-basic as 10oooooo


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
	uint16_t a_val, b_val;
	uint32_t result; // holds O and the value for math

	dcpu_inst_parse(cpu->RAM[cpu->PC++], &op, &a, &b);

	a_ptr = dcpu_ref(cpu, a); // a is always handled by the processor before b
	a_val = (a_ptr != NULL) ? *a_ptr : dcpu_get(cpu, a);
	b_val = dcpu_get(cpu, b);
	// TODO: should we set O if a is a literal?

	switch (op) {
		case 0x01:// SET a, b - sets a to b
			// assignment fail silently if trying to assign to a literal (a_ptr is null)
			if (a_ptr != NULL) {
				*a_ptr = b_val;
				// TODO: should we clear out O?
			}
			break;
		case 0x02:// ADD a, b - sets a to a+b, sets O to 0x0001 if there is's an overflow, 0x0 othterwise
			if (a_ptr != NULL) {
				result = a_val + b_val;
				*a_ptr = (uint16_t)result;
				cpu->O = (uint16_t)((result >> 16) & 0xffff);
			}
			break;
		case 0x03:// SUB a, b - sets a to a-b, sets O to 0xffff if there is's an underflow, 0x0 othterwise
			if (a_ptr != NULL) {
				result = a_val - b_val;
				*a_ptr = (uint16_t)result;
				cpu->O = (uint16_t)((result >> 16) & 0xffff);
			}
			break;
		case 0x04:// MUL a, b - sets a to a*b, sets a to a*b, sets O to ((a*b)>>16)&0xffff
			if (a_ptr != NULL) {
				result = a_val * b_val;
				*a_ptr = (uint16_t)result;
				cpu->O = (uint16_t)((result >> 16) & 0xffff);
			}
			break;
		case 0x05:// DIV a, b - sets a to a/b, sets O to ((a<<16)/b)&0xffff. if b==0, sets a and O to 0 instead.
			if (a_ptr != NULL) {
				// check for divide by zero
				if (b_val == 0) {
					*a_ptr = 0;
					cpu->O = 0;
				} else {
					*a_ptr = a_val / b_val;
					cpu->O = ((((uint32_t)(a_val)) << 16) / b_val) & 0xffff;
				}
			}
			break;		
		case 0x06:// MOD a, b - sets a to a%b. if b==0, sets a to 0 instead.
			if (a_ptr != NULL) {
				// check for divide by zero
				if (b_val == 0) {
					*a_ptr = 0;
				} else {
					*a_ptr = a_val % b_val;
				}
			}
			break;
		case 0x07:// SHL a, b - sets a to a<<b, sets O to ((a<<b)>>16)&0xffff
			if (a_ptr != NULL) {
				*a_ptr = a_val << b_val;
				cpu->O = ((((uint32_t)a_val) << b_val) >> 16) & 0xffff;
			}
			break;
		case 0x08:// SHR a, b - sets a to a>>b, sets O to ((a<<16)>>b)&0xffff
			if (a_ptr != NULL) {
				*a_ptr = a_val >> b_val;
				cpu->O = ((((uint32_t)a_val) << 16) >> b_val) & 0xffff;
			}
			break;
		case 0x09:// AND a, b - sets a to a&b
			if (a_ptr != NULL) {
				*a_ptr = a_val & b_val;
			}
			break;
		case 0x0a:// BOR a, b - sets a to a|b
			if (a_ptr != NULL) {
				*a_ptr = a_val | b_val;
			}
			break;
		case 0x0b:// XOR a, b - sets a to a^b
			if (a_ptr != NULL) {
				*a_ptr = a_val ^ b_val;
			}
			break;
		// Control-Flow
		case 0x0c:// IFE a, b - performs next instruction only if a==b
			if (a_val != b_val) {
				// skip next instruction
				dcpu_inst_parse(cpu->RAM[cpu->PC++], &op, &a, &b);
				dcpu_get(cpu, a); // consume next word(s) as needed
				dcpu_get(cpu, b);
			}
			break;
		case 0x0d:// IFN a, b - performs next instruction only if a!=b
			if (a_val == b_val) {
				// skip next instruction
				dcpu_inst_parse(cpu->RAM[cpu->PC++], &op, &a, &b);
				dcpu_get(cpu, a); // consume next word(s) as needed
				dcpu_get(cpu, b);
			}
			break;
		case 0x0e:// IFG a, b - performs next instruction only if a>b
			if (a_val <= b_val) {
				// skip next instruction
				dcpu_inst_parse(cpu->RAM[cpu->PC++], &op, &a, &b);
				dcpu_get(cpu, a); // consume next word(s) as needed
				dcpu_get(cpu, b);
			}
			break;
		case 0x0f:// IFB a, b - performs next instruction only if (a&b)!=0
			if ((a_val & b_val) == 0) {
				// skip next instruction
				dcpu_inst_parse(cpu->RAM[cpu->PC++], &op, &a, &b);
				dcpu_get(cpu, a); // consume next word(s) as needed
				dcpu_get(cpu, b);
			}
			break;
		// Non-basic opcodes
		case (0x01 | 0x80)://JSR a - pushes the address of the next instruction to the stack, then sets PC to a
			cpu->RAM[--(cpu->SP)] = cpu->PC;
			cpu->PC = a_val;
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
