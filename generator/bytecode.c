#include "bytecode.h"
#include "../../runtime/classes/string.h"
#include "../../runtime/classes/fixnum.h"

const char *variable_name(rt_value var)
{
	if(!var)
		return "<nil>";

	rt_value string = rt_string_from_cstr("");

	variable_t *_var = (variable_t *)var;

	switch(_var->type)
	{
		case V_TEMP:
			rt_concat_string(string, rt_string_from_cstr("%"));
			rt_concat_string(string, rt_fixnum_to_s(RT_INT2FIX(_var->index), 0, 0, 0));
			break;

		case V_ARGS:
			rt_concat_string(string, rt_string_from_cstr("<a:"));
			rt_concat_string(string, rt_fixnum_to_s(RT_INT2FIX(_var->index), 0, 0, 0));
			rt_concat_string(string, rt_string_from_cstr(">"));
			break;

		case V_PARAMETER:
			rt_concat_string(string, rt_string_from_cstr("*"));
			rt_concat_string(string, rt_symbol_to_s(_var->name, 0, 0, 0));
			break;

		case V_LOCAL:
			rt_concat_string(string, rt_string_from_cstr("#"));
			rt_concat_string(string, rt_symbol_to_s(_var->name, 0, 0, 0));
			break;

		case V_UPVAL:
			rt_concat_string(string, rt_string_from_cstr("!"));
			rt_concat_string(string, rt_symbol_to_s(_var->name, 0, 0, 0));
			break;

		case V_BLOCK:
			rt_concat_string(string, rt_string_from_cstr("&"));

			if(_var->name)
				rt_concat_string(string, rt_symbol_to_s(_var->name, 0, 0, 0));

			break;
	}

	return rt_string_to_cstr(string);
}

void opcode_print(opcode_t *op)
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

		case B_GET_UPVAL:
			printf("get_upval %s, %s", variable_name(op->result), variable_name(op->left));
			break;

		case B_SET_UPVAL:
			printf("set_upval %s, %s", variable_name(op->result), variable_name(op->left));
			break;

		case B_GET_IVAR:
			printf("get_ivar %s, %s", variable_name(op->result), rt_symbol_to_cstr(op->left));
			break;

		case B_SET_IVAR:
			printf("set_ivar %s, %s", rt_symbol_to_cstr(op->result), variable_name(op->left));
			break;

		case B_PUSH_UPVAL:
			printf("push_upval %s", variable_name(op->result));
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
			printf("#%d:", op->result);
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
			if(op->type == B_LABEL)
				printf("\n");

			opcode_print(kv_A(block->vector, i));
			printf("\n");
		}
	}
}
