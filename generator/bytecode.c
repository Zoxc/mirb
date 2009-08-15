#include "bytecode.h"
#include "../runtime/string.h"

void opcode_print(opcode_t *op)
{
	switch(op->type)
	{
		case B_NOP:
			printf("nop");
			break;

		case B_MOV_IMM:
			printf("mov "); block_print_var(op->result); printf(", %s", rt_string_to_cstr(rt_inspect(op->left)));
			break;

		case B_MOV:
			printf("mov "); block_print_var(op->result); printf(", "); block_print_var(op->left);
			break;

		case B_PUSH:
			printf("push "); block_print_var(op->result);
			break;

		case B_PUSH_IMM:
			printf("push %s", rt_string_to_cstr(rt_inspect(op->result)));
			break;

		case B_CALL:
			printf("call %s, %d", rt_symbol_to_cstr(op->result), op->left);
			break;

		case B_STORE:
			printf("store "); block_print_var(op->result);
			break;

		case B_SELF:
			printf("self "); block_print_var(op->result);
			break;

		case B_LOAD:
			printf("load "); block_print_var(op->result);
			break;

		case B_TEST:
			printf("test "); block_print_var(op->result);
			break;

		case B_TEST_IMM:
			printf("test %d", op->result);
			break;

		case B_JMPF:
			printf("jmpf "); block_print_label(op->result);
			break;

		case B_JMPT:
			printf("jmpt "); block_print_label(op->result);
			break;

		case B_JMP:
			printf("jmp "); block_print_label(op->result);
			break;

		case B_LABEL:
			printf("#%d:", op->result);
			break;

		case B_STRING:
			printf("string "); block_print_var(op->result); printf(", \"%s\"", op->left);
			break;

		case B_INTERPOLATE:
			printf("interpolate "); block_print_var(op->result); printf(", %d", op->left);
			break;

		case B_SET_CONST:
			printf("set_const "); block_print_var(op->result); printf(".%s", rt_symbol_to_cstr(op->left)); printf(", "); block_print_var(op->right);
			break;

		case B_GET_CONST:
			printf("get_const "); block_print_var(op->result); printf(", "); block_print_var(op->left); printf(".%s", rt_symbol_to_cstr(op->right));
			break;

		case B_CLASS:
			printf("class %s, %x", rt_symbol_to_cstr(op->result), op->left);
			break;

		case B_METHOD:
			printf("method %s, %x", rt_symbol_to_cstr(op->result), op->left);
			break;

		default:
			printf("unknown opcode %d", op->type);
	}
}

void block_print(block_t *block)
{
	for(size_t i = 0; i < kv_size(block->vector); i++)
	{
		opcode_t *op = kv_A(block->vector, i);

		if(op->type != B_NOP)
		{
			opcode_print(kv_A(block->vector, i));
			printf("\n");
		}
	}
}
