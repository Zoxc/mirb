#include "block.h"
#include "../runtime/classes/string.h"
#include "../runtime/classes/symbol.h"

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
