#include "bytecode.h"
#include "block.h"
#include "../runtime/classes/string.h"
#include "../runtime/classes/fixnum.h"
#include "../runtime/classes/symbol.h"

void opcode_print(struct opcode *op)
{
	switch(op->type)
	{
		case B_NOP:
			printf("nop");
			break;

		case B_MOV_IMM:
			printf("mov %s, %s", variable_name(op->result), rt_string_to_cstr(rt_inspect(op->left)));
			break;

		case B_MOV:
			printf("mov %s, %s", variable_name(op->result), variable_name(op->left));
			break;

		case B_PUSH:
			printf("push %s", variable_name(op->result));
			break;

		case B_GET_HVAR:
			printf("get_hvar %s, %s", variable_name(op->result), variable_name(op->left));
			break;

		case B_SET_HVAR:
			printf("set_hvar %s, %s", variable_name(op->result), variable_name(op->left));
			break;

		case B_GET_IVAR:
			printf("get_ivar %s, %s", variable_name(op->result), rt_symbol_to_cstr(op->left));
			break;

		case B_SET_IVAR:
			printf("set_ivar %s, %s", rt_symbol_to_cstr(op->result), variable_name(op->left));
			break;

		case B_PUSH_SCOPE:
			printf("push_scope %s", variable_name(op->result));
			break;

		case B_PUSH_RAW:
			printf("push %d", op->result);
			break;

		case B_PUSH_IMM:
			printf("push %s", rt_string_to_cstr(rt_inspect(op->result)));
			break;

		case B_SEAL:
			printf("seal %s", variable_name(op->result));
			break;

		case B_UPVAL:
			printf("upval %s, %s", variable_name(op->result), variable_name(op->left));
			break;

		case B_CLOSURE:
			printf("closure %s, %x", variable_name(op->result), op->left);
			break;

		case B_CALL:
			printf("call %s, %s, %s", variable_name(op->result), rt_symbol_to_cstr(op->left), variable_name(op->right));
			break;

		case B_STORE:
			printf("store %s", variable_name(op->result));
			break;

		case B_SELF:
			printf("self %s", variable_name(op->result));
			break;

		case B_LOAD:
			printf("load %s", variable_name(op->result));
			break;

		case B_TEST:
			printf("test %s", variable_name(op->result));
			break;

		case B_CMP:
			printf("cmp %s, %s", variable_name(op->result), variable_name(op->left));
			break;

		case B_ARGS:
			if(op->left)
				printf("end_args %s, %d", variable_name(op->result), op->right);
			else
				printf("start_args %s", variable_name(op->result));
			break;

		case B_JMPF:
			printf("jmpf "); block_print_label(op->result);
			break;

		case B_JMPT:
			printf("jmpt "); block_print_label(op->result);
			break;

		case B_JMPNE:
			printf("jmpne "); block_print_label(op->result);
			break;

		case B_JMPE:
			printf("jmpe "); block_print_label(op->result);
			break;

		case B_JMP:
			printf("jmp "); block_print_label(op->result);
			break;

		case B_ENSURE_RET:
			printf("ensure_ret");
			break;

		case B_HANDLER:
			printf("handler %d", op->result);
			break;

		case B_LABEL:
			block_print_label((rt_value)op);
			break;

		case B_STRING:
			printf("string %s, \"%s\"", variable_name(op->result), (char *)op->left);
			break;

		case B_INTERPOLATE:
			printf("interpolate %s", variable_name(op->result));
			break;

		case B_ARRAY:
			printf("array %s", variable_name(op->result));
			break;

		case B_SET_CONST:
			printf("set_const %s.%s, %s", variable_name(op->result), rt_symbol_to_cstr(op->left), variable_name(op->right));
			break;

		case B_GET_CONST:
			printf("get_const %s, %s.%s", variable_name(op->result), variable_name(op->left), rt_symbol_to_cstr(op->right));
			break;

		case B_CLASS:
			printf("class %s, %x", rt_symbol_to_cstr(op->result), op->left);
			break;

		case B_MODULE:
			printf("module %s, %x", rt_symbol_to_cstr(op->result), op->left);
			break;

		case B_METHOD:
			printf("method %s, %x", rt_symbol_to_cstr(op->result), op->left);
			break;

		case B_RETURN:
			printf("return %s", variable_name(op->result));
			break;

		case B_REDO:
			printf("redo");
			break;

		case B_RAISE_RETURN:
			printf("raise_return %s, %d", variable_name(op->result), op->left);
			break;

		case B_RAISE_BREAK:
			printf("raise_break %s, %d, %d", variable_name(op->result), op->left, op->right);
			break;

		default:
			printf("unknown opcode %d", op->type);
	}
}
