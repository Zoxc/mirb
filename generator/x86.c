#include "x86.h"
#include "../runtime/x86.h"
#include "../runtime/support.h"
#include "../runtime/code_heap.h"
#include "../runtime/method.h"
#include "../runtime/constant.h"
#include "disassembly.h"

static inline void generate_byte(unsigned char **target, unsigned char byte)
{
	**target = byte;
	(*target)++;
}

static inline void generate_word(unsigned char **target, unsigned short word)
{
	**(unsigned short**)target = word;
	*target += sizeof(word);
}

static inline void generate_dword(unsigned char **target, size_t dword)
{
	**(size_t**)target = dword;
	*target += sizeof(dword);
}

static inline size_t instruction_size(block_t *block, opcode_t *op, size_t i, size_t current)
{
	switch(op->type)
	{
		case B_LABEL:
			op->right = (rt_value)current;
			return 0;

		case B_NOP:
			return 0;

		case B_MOV_IMM:
			return 7;

		case B_MOV:
			return 6;

		case B_CALL:
			return (op->right ? 3 : 5) + 3 + 5 + 5 + 2 + (kv_A(block->vector, i - 1)->right ? 6 : 0);

		case B_PUSH:
			return 3;

		case B_PUSH_RAW:
		case B_PUSH_IMM:
			return 5;

		case B_SELF:
		case B_LOAD:
		case B_STORE:
			return 3;

			return 3;

		case B_TEST:
			return 3 + 5;

		case B_JMP:
			return 5;

		case B_JMPT:
		case B_JMPF:
			return 6;

		case B_MODULE:
			return 5 + 5 + 5 + 1 + 5 + 6;

		case B_CLASS:
			return 5 + 5 + 5 + 5 + 1 + 5 + 6;

		case B_METHOD:
			return 5 + 5 + 5;

		case B_GET_CONST:
			return 5 + 3 + 5 + 3;

		case B_SET_CONST:
			return 3 + 5 + 3 + 5;

		case B_STRING:
			return 5 + 5 + 3;

		case B_INTERPOLATE:
			return 5 + 6 + 3;

		case B_UPVAL:
			return 3 + 5 + 3;

		case B_CLOSURE:
			return 5 + 5 + 6 + 3;

		case B_GET_UPVAL:
			return 5 + 5 + 3;

		case B_SET_UPVAL:
			return 5 + 3 + 5;

		case B_SEAL:
			return 3 + 5;

		case B_ARGS:
			if(op->left)
				return 1 + 5;
			else
				return 0;

		default:
			break;
	}

	return 0;
}

static inline int get_stack_index(block_t *block, rt_value var)
{
	variable_t *_var = (variable_t *)var;

	int index;

	switch(_var->type)
	{
		case V_BLOCK:
			return 12;

		case V_PARAMETER:
			index = block->scope->var_count[V_LOCAL] + block->scope->var_count[V_TEMP] + _var->index;
			break;

		case V_TEMP:
			index = block->scope->var_count[V_LOCAL] + _var->index;
			break;

		case V_LOCAL:
			index = _var->index;
			break;

		default:
			assert(0);
	}

	return -((index + 1) * 4);
}

static inline void generate_call(unsigned char **target, void *function)
{
	generate_byte(target, 0xE8);
	generate_dword(target, (size_t)function - ((size_t)*target - 1) - 5);
}

static inline void generate_stack_push(unsigned char **target, size_t dword)
{
	generate_byte(target, 0x68);
	generate_dword(target, dword);
}

static inline void generate_stack_var_push(block_t *block, unsigned char **target, rt_value var)
{
	generate_byte(target, 0xFF);
	generate_byte(target, 0x75);
	generate_byte(target, (char)get_stack_index(block, var));
}

static inline void generate_stack_pop(unsigned char **target, size_t bytes)
{
	generate_byte(target, 0x81);
	generate_byte(target, 0xC4);
	generate_dword(target, bytes);
}

static inline void generate_instruction(block_t *block, opcode_t *op, size_t i, unsigned char *start, unsigned char **target)
{
	switch(op->type)
	{
		case B_SEAL:
			{
				generate_stack_var_push(block, target, op->result);
				generate_call(target, rt_support_seal_upval);
			}
			break;

		case B_GET_UPVAL:
			{
				generate_byte(target, 0xB8); // mov eax,
				generate_dword(target, ((variable_t *)op->left)->index);

				generate_call(target, rt_support_get_upval);

				generate_byte(target, 0x89);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));
			}
			break;

		case B_SET_UPVAL:
			{
				generate_byte(target, 0xB8); // mov eax,
				generate_dword(target, ((variable_t *)op->result)->index);

				generate_stack_var_push(block, target, op->left);

				generate_call(target, rt_support_set_upval);
			}
			break;

		case B_CLOSURE:
			{
				generate_stack_push(target, (size_t)compile_block((block_t *)op->left));

				generate_call(target, rt_support_closure);

				generate_stack_pop(target, (kv_A(block->vector, i - 1)->right + 3) * 4);

				generate_byte(target, 0x89);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));
			}
			break;

		case B_UPVAL:
			{
				generate_byte(target, 0x8D);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->left));

				generate_call(target, rt_support_upval_create);

				generate_byte(target, 0x89);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));
			}
			break;

		case B_CLASS:
			{
				block_t *class_block = (block_t *)op->left;
				rt_compiled_block_t compiled = compile_block(class_block);

				generate_stack_push(target, rt_Object);
				generate_stack_push(target, op->result);
				generate_call(target, rt_support_define_class);

				generate_byte(target, 0x50); // push dummy argv
				generate_stack_push(target, 0); // push argc
				generate_stack_push(target, 0); // push block
				generate_byte(target, 0x50); // push obj(eax)

				generate_call(target, compiled);
			}
			break;

		case B_MODULE:
			{
				block_t *class_block = (block_t *)op->left;
				rt_compiled_block_t compiled = compile_block(class_block);

				generate_stack_push(target, op->result);
				generate_call(target, rt_support_define_module);

				generate_byte(target, 0x50); // push dummy argv
				generate_stack_push(target, 0); // push argc
				generate_stack_push(target, 0); // push block
				generate_byte(target, 0x50); // push obj(eax)

				generate_call(target, compiled);
			}
			break;

		case B_GET_CONST:
			{
				generate_stack_push(target, op->right);
				generate_stack_var_push(block, target, op->left);

				generate_call(target, rt_support_get_const);

				generate_byte(target, 0x89);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));
			}
			break;

		case B_SET_CONST:
			{
				generate_stack_var_push(block, target, op->right);
				generate_stack_push(target, op->left);
				generate_stack_var_push(block, target, op->result);

				generate_call(target, rt_support_set_const);
			}
			break;

		case B_METHOD:
			{
				block_t *method_block = (block_t *)op->left;
				rt_compiled_block_t compiled = compile_block(method_block);

				generate_stack_push(target, (rt_value)compiled);
				generate_stack_push(target, op->result);
				//generate_byte(target, 0x57); // push edi

				generate_call(target, rt_support_define_method);
			}
			break;

		case B_CALL:
			{
				if(op->right)
					generate_stack_var_push(block, target, op->right);
				else
					generate_stack_push(target, 0);

				generate_stack_var_push(block, target, op->result);

				generate_byte(target, 0xB8); // mov eax,
				generate_dword(target, op->left);

				generate_call(target, rt_support_lookup_method);

				generate_byte(target, 0xFF); // call eax
				generate_byte(target, 0xD0);

				if(kv_A(block->vector, i - 1)->right)
					generate_stack_pop(target, kv_A(block->vector, i - 1)->right * 4);
			}
			break;

		case B_ARGS:
			{
				if(op->left)
				{
					generate_byte(target, 0x54); // push esp
					generate_stack_push(target, op->right);
				}
			}
			break;

		case B_STRING:
			{
				generate_stack_push(target, op->left);
				generate_call(target, rt_support_define_string);

				generate_byte(target, 0x89);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));
			}
			break;

		case B_INTERPOLATE:
			{
				generate_call(target, rt_support_interpolate);

				generate_stack_pop(target, (kv_A(block->vector, i - 1)->right + 2) * 4);

				generate_byte(target, 0x89);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));
			}
			break;

		case B_PUSH:
			generate_stack_var_push(block, target, op->result);
			break;

		case B_PUSH_RAW:
		case B_PUSH_IMM:
			generate_stack_push(target, op->result);
			break;

		case B_SELF:
			{
				generate_byte(target, 0x89);
				generate_byte(target, 0x7D);
				generate_byte(target, (char)get_stack_index(block, op->result));
			}
			break;

		case B_LOAD:
			{
				generate_byte(target, 0x8B);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));
			}
			break;

		case B_STORE:
			{
				generate_byte(target, 0x89);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));
			}
			break;

		case B_JMP:
			{
				size_t label_index = block_get_value(block, block->label_usage, op->result);

				opcode_t *label = kv_A(block->vector, label_index);

				size_t label_address = (size_t)start + label->right;

				generate_byte(target, 0xE9);
				generate_dword(target, label_address - ((size_t)*target - 1) - 5);
			}
			break;

		case B_JMPT:
			{
				size_t label_index = block_get_value(block, block->label_usage, op->result);

				opcode_t *label = kv_A(block->vector, label_index);

				size_t label_address = (size_t)start + label->right;

				generate_byte(target, 0x0F);
				generate_byte(target, 0x85);
				generate_dword(target, label_address - ((size_t)*target - 2) - 6);
			}
			break;

		case B_JMPF:
			{
				size_t label_index = block_get_value(block, block->label_usage, op->result);

				opcode_t *label = kv_A(block->vector, label_index);

				size_t label_address = (size_t)start + label->right;

				generate_byte(target, 0x0F);
				generate_byte(target, 0x84);
				generate_dword(target, label_address - ((size_t)*target - 2) - 6);
			}
			break;

		case B_TEST:
			{
				// Load to eax
				generate_byte(target, 0x8B);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));

				// and eax, -5

				generate_byte(target, 0x25);
				generate_dword(target, -5);
			}
			break;

		case B_MOV:
			{
				// Load to eax
				{
					generate_byte(target, 0x8B);
					generate_byte(target, 0x45);
					generate_byte(target, (char)get_stack_index(block, op->left));
				}

				// Store from eax
				{
					generate_byte(target, 0x89);
					generate_byte(target, 0x45);
					generate_byte(target, (char)get_stack_index(block, op->result));
				}
			}
			break;

		case B_MOV_IMM:
			{
				generate_byte(target, 0xC7);
				generate_byte(target, 0x45);
				generate_byte(target, (char)get_stack_index(block, op->result));

				generate_dword(target, op->left);
			}
			break;

		default:
			break;
	}
}

rt_compiled_block_t compile_block(block_t *block)
{
	size_t block_size = 3;
	size_t stack_vars = block->scope->var_count[V_LOCAL] + block->scope->var_count[V_TEMP] + block->scope->var_count[V_PARAMETER];

	printf("Compiling block %x:\n", (rt_value)block);

	if(stack_vars > 0)
		block_size += 6;

	if(block->scope->type == S_CLOSURE)
		block_size += 3;

	if(block->self_ref > 0)
		block_size += 4;

	if(block->scope->var_count[V_PARAMETER])
	{
		khash_t(scope) *variables = block->scope->variables;

		block_size += 3;

		for(khiter_t k = kh_begin(variables); k != kh_end(variables); ++k)
		{
			if(kh_exist(variables, k) && kh_value(variables, k)->type == V_PARAMETER)
				block_size += 6;
		}
	}

	for(size_t i = 0; i < kv_size(block->vector); i++)
		block_size += instruction_size(block, kv_A(block->vector, i), i, block_size);

	if(block->self_ref > 0)
		block_size += 1;

	if(block->scope->type == S_CLOSURE)
		block_size += 1;

	block_size += 6;

	rt_compiled_block_t result = rt_code_heap_alloc(block_size);

	unsigned char *target = (unsigned char *)result;

	generate_byte(&target, 0x55);
	generate_byte(&target, 0x89);
	generate_byte(&target, 0xE5);

	if(stack_vars > 0)
	{
		generate_byte(&target, 0x81);
		generate_byte(&target, 0xEC);
		generate_dword(&target, stack_vars * 4);
	}

	if(block->scope->type == S_CLOSURE)
	{
		generate_byte(&target, 0x56); // push esi

		generate_byte(&target, 0x89); // mov esi, eax
		generate_byte(&target, 0xC6);
	}

	if(block->self_ref > 0)
	{
		generate_byte(&target, 0x57);
		generate_byte(&target, 0x8B);
		generate_byte(&target, 0x7D);
		generate_byte(&target, 8);
	}

	if(block->scope->var_count[V_PARAMETER])
	{
		khash_t(scope) *variables = block->scope->variables;

		generate_byte(&target, 0x8B);
		generate_byte(&target, 0x4D);
		generate_byte(&target, (char)20);

		for(khiter_t k = kh_begin(variables); k != kh_end(variables); ++k)
		{
			if(kh_exist(variables, k))
			{
				variable_t *var = kh_value(variables, k);

				if(var->type == V_PARAMETER)
				{
					// Load to edx
					generate_byte(&target, 0x8B);
					generate_byte(&target, 0x51);
					generate_byte(&target, (char)((block->scope->var_count[V_PARAMETER] - var->index - 1) * 4));

					// Store from edx
					generate_byte(&target, 0x89);
					generate_byte(&target, 0x55);
					generate_byte(&target, (char)get_stack_index(block, (rt_value)var));
				}
			}
		}
	}

	for(size_t i = 0; i < kv_size(block->vector); i++)
		generate_instruction(block, kv_A(block->vector, i), i, (unsigned char *)result, &target);

	if(block->self_ref > 0)
		generate_byte(&target, 0x5F);

	if(block->scope->type == S_CLOSURE)
		generate_byte(&target, 0x5E); // pop esi

	generate_byte(&target, 0x89);
	generate_byte(&target, 0xEC);
	generate_byte(&target, 0x5D);
	generate_byte(&target, 0xC2);
	generate_word(&target, 16);

    #ifdef WINDOWS
        dump_code((void *)result, target - (unsigned char *)result);
	#endif

	assert((unsigned char *)result + block_size == target);

	printf("End compiling block %x:\n", (rt_value)block);

	return result;
}
