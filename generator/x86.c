#include "x86.h"
#include "../runtime/x86.h"
#include "../runtime/code_heap.h"
#include "../runtime/method.h"
#include "../runtime/constant.h"
#include "disassembly.h"

typedef struct {
	int base : 3;
	int index : 3;
	unsigned int scale : 2;
} sib_t;

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

static inline void generate_dword(unsigned char **target, unsigned int dword)
{
	**(unsigned int**)target = dword;
	*target += sizeof(dword);
}

static inline unsigned int instruction_size(block_t *block, opcode_t *op, size_t i, size_t current)
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
			return 5 + 3 + 6;

		case B_PUSH:
			return 3;

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

		case B_CLASS:
			return 5 + 5 + 5 + 1 + 5 + 1 + 5 + 6;

		case B_CLASS_MAIN:
			return 5 + 5 + 5 + 5 + 1 + 5 + 6;

		case B_METHOD:
			return 5 + 5 + 1 + 5;

		case B_METHOD_MAIN:
			return 5 + 5 + 5;

		case B_GET_CONST:
			return 5 + 3 + 5 + 3;

		case B_SET_CONST:
			return 3 + 5 + 3 + 5;

		default:
			break;
	}

	return 0;
}

static inline int get_stack_index(block_t *block, rt_value var)
{
	unsigned int next_param = block->scope->next_param;

	if(var < next_param)
		return 8 + ((next_param - 1) * 4 -  var * 4);
	else
		return -(var * 4);
}

static inline void generate_call(unsigned char **target, void *function)
{
	generate_byte(target, 0xE8);
	generate_dword(target, (unsigned int)function - ((unsigned int)*target - 1) - 5);
}

static inline void generate_stack_push(unsigned char **target, unsigned int dword)
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

static inline void generate_stack_pop(unsigned char **target, unsigned int bytes)
{
	generate_byte(target, 0x81);
	generate_byte(target, 0xC4);
	generate_dword(target, bytes);
}

static inline void generate_instruction(block_t *block, opcode_t *op, size_t i, unsigned char *start, unsigned char **target)
{
	switch(op->type)
	{
		case B_CLASS:
			{
				block_t *class_block = (block_t *)op->left;
				rt_compiled_block_t compiled = compile_block(class_block);

				generate_stack_push(target, rt_Object);
				generate_stack_push(target, op->result);
				generate_byte(target, 0x57); // push edi

				generate_call(target, rt_support_class_create);

				generate_stack_push(target, 1);
				generate_byte(target, 0x50); // push eax

				generate_call(target, compiled);

				generate_stack_pop(target, 4);
			}
			break;

		case B_CLASS_MAIN:
			{
				block_t *class_block = (block_t *)op->left;
				rt_compiled_block_t compiled = compile_block(class_block);

				generate_stack_push(target, rt_Object);
				generate_stack_push(target, op->result);

				generate_call(target, rt_support_class_create_main);

				generate_stack_push(target, 1);
				generate_byte(target, 0x50); // push eax

				generate_call(target, compiled);

				generate_stack_pop(target, 4);
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
				generate_byte(target, 0x57); // push edi

				generate_call(target, rt_method_create);
			}
			break;

		case B_METHOD_MAIN:
			{
				block_t *method_block = (block_t *)op->left;
				rt_compiled_block_t compiled = compile_block(method_block);

				generate_stack_push(target, (rt_value)compiled);
				generate_stack_push(target, op->result);

				generate_call(target, rt_method_create_main);
			}
			break;

		case B_CALL:
			{
				generate_call(target, rt_lookup);

				generate_byte(target, 0x59); // pop ecx

				generate_byte(target, 0xFF); // call eax
				generate_byte(target, 0xD0);

				generate_stack_pop(target, op->result * 4);
			}
			break;

		case B_PUSH:
			generate_stack_var_push(block, target, op->result);
			break;

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
				unsigned int label_index = block_get_value(block, block->label_usage, op->result);

				opcode_t *label = kv_A(block->vector, label_index);

				unsigned int label_address = (unsigned int)start + label->right;

				generate_byte(target, 0xE9);
				generate_dword(target, label_address - ((unsigned int)*target - 1) - 5);
			}
			break;

		case B_JMPT:
			{
				unsigned int label_index = block_get_value(block, block->label_usage, op->result);

				opcode_t *label = kv_A(block->vector, label_index);

				unsigned int label_address = (unsigned int)start + label->right;

				generate_byte(target, 0x0F);
				generate_byte(target, 0x85);
				generate_dword(target, label_address - ((unsigned int)*target - 2) - 6);
			}
			break;

		case B_JMPF:
			{
				unsigned int label_index = block_get_value(block, block->label_usage, op->result);

				opcode_t *label = kv_A(block->vector, label_index);

				unsigned int label_address = (unsigned int)start + label->right;

				generate_byte(target, 0x0F);
				generate_byte(target, 0x84);
				generate_dword(target, label_address - ((unsigned int)*target - 2) - 6);
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
	unsigned int block_size = 3;
	unsigned int stack_vars = (block->next_temp - 1) - (block->scope->next_param - 1);

	printf("Compiling block %x:\n", block);

	if (stack_vars > 0)
		block_size += 6;

	if (block->self_ref > 0)
		block_size += 4;

	for (size_t i = 0; i < kv_size(block->vector); i++)
		block_size += instruction_size(block, kv_A(block->vector, i), i, block_size);

	if (block->self_ref > 0)
		block_size += 1;

	block_size += 4;

	rt_compiled_block_t result = code_heap_alloc(block_size);

	unsigned char *target = (unsigned char *)result;

	generate_byte(&target, 0x55);
	generate_byte(&target, 0x89);
	generate_byte(&target, 0xE5);

	if (block->self_ref > 0)
	{
		generate_byte(&target, 0x57);
		generate_byte(&target, 0x8B);
		generate_byte(&target, 0x7D);
		generate_byte(&target, 8);
	}

	if (stack_vars > 0)
	{
		generate_byte(&target, 0x81);
		generate_byte(&target, 0xEC);
		generate_dword(&target, (block->next_temp - 1) * 4);
	}

	for (size_t i = 0; i < kv_size(block->vector); i++)
		generate_instruction(block, kv_A(block->vector, i), i, (unsigned char *)result, &target);

	if (block->self_ref > 0)
		generate_byte(&target, 0x5F);

	generate_byte(&target, 0x89);
	generate_byte(&target, 0xEC);
	generate_byte(&target, 0x5D);
	generate_byte(&target, 0xC3);

	dump_code((void *)result, target - (unsigned char *)result);

	assert((unsigned char *)result + block_size == target);

	printf("End compiling block %x:\n", block);

	return result;
}
