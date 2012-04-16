#include "dcpu.h"


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

	// // underflow
	// // SET A, 1
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x0, 0x21);
	// dcpu_exec1(cpu);
	// dcpu_print_state(cpu);
	// // ADD A, 2
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x3, 0x0, 0x22);
	// dcpu_exec1(cpu);
	// dcpu_print_state(cpu);

	// // MUL
	// // SET A, 3
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x0, 0x23);
	// dcpu_exec1(cpu);
	// dcpu_print_state(cpu);
	// // SET B, 4
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x1, 0x24);
	// dcpu_exec1(cpu);
	// dcpu_print_state(cpu);
	// // MUL A, B
	// cpu->RAM[cpu->PC] = dcpu_inst_make(0x4, 0x0, 0x1);
	// dcpu_exec1(cpu);
	// dcpu_print_state(cpu);

	// DIV
	// SET A, 6
	cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x0, 0x26);
	dcpu_exec1(cpu);
	dcpu_print_state(cpu);
	// SET B, 2
	cpu->RAM[cpu->PC] = dcpu_inst_make(0x1, 0x1, 0x22);
	dcpu_exec1(cpu);
	dcpu_print_state(cpu);
	// DIV A, B
	cpu->RAM[cpu->PC] = dcpu_inst_make(0x5, 0x0, 0x1);
	dcpu_exec1(cpu);
	dcpu_print_state(cpu);




	return 0;
}

