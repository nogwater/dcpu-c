#ifndef __dcpu_h__
#define __dcpu_h__


#include <stdint.h>

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


dcpu_t *dcpu_make();
uint16_t dcpu_inst_make(uint8_t op, uint8_t a, uint8_t b);
// void dcpu_inst_parse(uint16_t word, uint8_t *op, uint8_t *a, uint8_t *b);
// uint16_t *dcpu_ref(dcpu_t *cpu, uint8_t ref);
// uint16_t dcpu_get(dcpu_t *cpu, uint8_t ref);
void dcpu_exec1(dcpu_t *cpu);
void dcpu_print_state(dcpu_t *cpu);
void dcpu_print_video(dcpu_t *cpu);
void dcpu_print_stack(dcpu_t *cpu);

void print_bits(uint16_t word);

#endif