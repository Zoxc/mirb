#include "block.hpp"
#include "../runtime/classes/string.hpp"
#include "../runtime/classes/symbol.hpp"

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

#ifdef DEBUG
	void block_print(struct block *block)
	{
		for(size_t i = 0; i < block->opcodes.size; i++)
		{
			struct opcode *op = block->opcodes.array[i];

			if(op->type != B_NOP)
			{
				if(op->type == B_LABEL)
					printf("\n");

				opcode_print(block->opcodes.array[i]);
				printf("\n");
			}
		}
	}
#endif

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
	for(size_t i = 0; i < owner->parameters.size; i++)
	{
		block_require_var(current, owner->parameters.array[i]);
	}

	if(owner->block_parameter)
		block_require_var(current, owner->block_parameter);
}

struct block *block_create(struct compiler *compiler, enum block_type type)
{
	struct block *result = (struct block *)compiler_alloc(compiler, sizeof(struct block));

	result->data = (struct block_data *)malloc(sizeof(struct block_data));

	result->compiler = compiler;

	#ifdef DEBUG
		result->label_count = 0;
	#endif

	result->output = 0;
	result->self_ref = 0;
	result->require_exceptions = false;
	result->current_exception_block = 0;
	result->current_exception_block_id = (size_t)-1;
	result->break_targets = 0;
	result->can_break = false;
	result->heap_vars = 0;

	vec_init(variables, &result->parameters, &compiler->allocator);
	vec_init(opcodes, &result->opcodes, &compiler->allocator);
	vec_init(rt_exception_blocks, &result->exception_blocks);
	vec_init(blocks, &result->scopes, &compiler->allocator);
	vec_init(blocks, &result->zsupers, &compiler->allocator);

	result->variables = hash_init(block, &compiler->allocator);

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

