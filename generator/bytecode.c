#include "bytecode.h"

void opcode_print(opcode_t *op)
{
	switch(op->type)
	{
		case B_NOP:
			printf("nop\n");
			break;

		case B_MOV_IMM:
			printf("mov "); block_print_var(op->result); printf(", %d\n", op->left);
			break;

		case B_MOV:
			printf("mov "); block_print_var(op->result); printf(", "); block_print_var(op->left);  printf("\n");
			break;

		case B_PUSH:
			printf("push "); block_print_var(op->result); printf("\n");
			break;

		case B_PUSH_IMM:
			printf("push %d\n", op->result);
			break;

		case B_CALL:
			printf("call %d\n", op->result);
			break;

		case B_STORE:
			printf("store "); block_print_var(op->result); printf("\n");
			break;

		case B_SELF:
			printf("self "); block_print_var(op->result); printf("\n");
			break;

		case B_LOAD:
			printf("load "); block_print_var(op->result); printf("\n");
			break;

		case B_TEST:
			printf("test "); block_print_var(op->result); printf("\n");
			break;

		case B_TEST_IMM:
			printf("test %d", op->result); printf("\n");
			break;

		case B_JMPF:
			printf("jmpf "); block_print_label(op->result); printf("\n");
			break;

		case B_JMPT:
			printf("jmpt "); block_print_label(op->result); printf("\n");
			break;

		case B_JMP:
			printf("jmp "); block_print_label(op->result); printf("\n");
			break;

		case B_LABEL:
			printf("#%d:\n", op->result);
			break;

		case B_SET_CONST:
			printf("set_const "); block_print_var(op->result); printf("."); rt_symbol_to_cstr(op->left); printf(", "); block_print_var(op->right); printf("\n");
			break;

		case B_GET_CONST:
			printf("get_const "); block_print_var(op->result); printf(", "); block_print_var(op->left); printf("."); rt_symbol_to_cstr(op->right); printf("\n");
			break;

		case B_CLASS:
			printf("class %s, %x\n", rt_symbol_to_cstr(op->result), op->left);
			break;

		case B_METHOD:
			printf("method %s, %x\n", rt_symbol_to_cstr(op->result), op->left);
			break;

		default:
			printf("unknown opcode %d\n", op->type);
	}
}

void block_print(block_t *block)
{
	for(size_t i = 0; i < kv_size(block->vector); i++)
	{
		opcode_t *op = kv_A(block->vector, i);

		if(op->type != B_NOP)
			opcode_print(kv_A(block->vector, i));
	}
}
