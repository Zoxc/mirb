#include "block.h"
#include "../runtime/classes/string.h"
#include "../runtime/classes/symbol.h"

const char *variable_name(rt_value var)
{
	if(!var)
		return "<nil>";

	rt_value string = rt_string_from_cstr("");

	struct variable *_var = (struct variable *)var;

	switch(_var->type)
	{
		case V_TEMP:
			rt_concat_string(string, rt_string_from_cstr("%"));
			rt_concat_string(string, rt_string_from_fixnum(RT_INT2FIX(_var->index)));
			break;

		case V_ARGS:
			rt_concat_string(string, rt_string_from_cstr("<a:"));
			rt_concat_string(string, rt_string_from_fixnum(RT_INT2FIX(_var->index)));
			rt_concat_string(string, rt_string_from_cstr(">"));
			break;

		case V_HEAP:
			if(_var->owner->block_parameter == _var)
				rt_concat_string(string, rt_string_from_cstr("!&"));
			else
				rt_concat_string(string, rt_string_from_cstr("!#"));

			if(_var->name)
				rt_concat_string(string, rt_string_from_symbol(_var->name));
			break;

		case V_LOCAL:
			if(_var->owner->block_parameter == _var)
				rt_concat_string(string, rt_string_from_cstr("&"));
			else
				rt_concat_string(string, rt_string_from_cstr("#"));

			if(_var->name)
				rt_concat_string(string, rt_string_from_symbol(_var->name));
			break;
	}

	return rt_string_to_cstr(string);
}

void block_print(struct block *block)
{
	for(size_t i = 0; i < kv_size(block->vector); i++)
	{
		struct opcode *op = kv_A(block->vector, i);

		if(op->type != B_NOP)
		{
			if(op->type == B_LABEL)
				printf("\n");

			opcode_print(kv_A(block->vector, i));
			printf("\n");
		}
	}
}

void block_require_var(struct block *current, struct variable *var)
{
	var->type = V_HEAP;
	var->owner->heap_vars = true;

	/*
	 * Make sure the block owning the varaible is required by the current block and parents.
	 */

	while(current != var->owner)
	{
		block_require_scope(current, var->owner);

		current = current->parent;
	}
}

void block_require_args(struct block *current, struct block *owner)
{
	for(size_t i = 0; i < kv_size(owner->parameters); i++)
	{
		block_require_var(current, kv_A(owner->parameters, i));

	}

	if(owner->block_parameter)
		block_require_var(current, owner->block_parameter);
}

struct block *block_create(struct compiler *compiler, enum block_type type)
{
	struct block *result = compiler_alloc(compiler, sizeof(struct block));

	result->data = malloc(sizeof(struct block_data));

	result->compiler = compiler;

	#ifdef DEBUG
		result->label_count = 0;
	#endif

	result->self_ref = 0;
	result->require_exceptions = false;
	result->current_exception_block = 0;
	result->current_exception_block_id = (size_t)-1;
	result->break_targets = 0;
	result->can_break = false;
	result->heap_vars = 0;

	kv_init(result->parameters);
	kv_init(result->vector);
	kv_init(result->exception_blocks);
	kv_init(result->scopes);
	kv_init(result->zsupers);

	result->variables = kh_init(block);

	for(int i = 0; i < VARIABLE_TYPES; i++)
		result->var_count[i] = 0;

	result->type = type;
	result->block_parameter = 0;
	result->super_module_var = 0;
	result->super_name_var = 0;
	result->scope_var = 0;
	result->closure_var = 0;

	result->owner = 0;
	result->parent = 0;

	return result;
}

