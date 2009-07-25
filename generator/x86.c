#include "x86.h"
#include "../runtime/code_heap.h"
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

		case B_PHI_IMM:
		case B_MOV_IMM:
			return 7;

		case B_PHI:
		case B_MOV:
			return 6;

		case B_CALL:
			return 5 + 3 + 6;

		case B_PUSH:
			return 3;

		case B_PUSH_IMM:
			return 5;

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

		default:
			break;
	}

	return 0;
}
/*
static inline void generate_jump(unsigned char c_short, unsigned char c_long, unsigned char *start, unsigned char **target, opcode_t *label)
{
	unsigned int label_address = (unsigned int)start + label->right;

	if(c_long != 0xE9)
	{
		generate_byte(target, 0x0F);

	generate_byte(target, c_long);
	generate_dword(target, label_address - ((unsigned int)*target - 1) - 5);


	int offset = label_address - ((unsigned int)*target - 1) - 2;

	printf("%x %x %d\n", label_address, *target, offset);

	if(offset <= 127 && offset >= -128)
		return true;
	else
		return false;
}
*/
static inline void generate_instruction(block_t *block, opcode_t *op, size_t i, unsigned char *start, unsigned char **target)
{
	switch(op->type)
	{
		case B_NOP:
			break;

		case B_CALL:
			{
				generate_byte(target, 0xE8);
				generate_dword(target, (unsigned int)rt_lookup - ((unsigned int)*target - 1) - 5);

				generate_byte(target, 0x59); // pop ecx

				generate_byte(target, 0xFF); // call eax
				generate_byte(target, 0xD0);

				generate_byte(target, 0x81);
				generate_byte(target, 0xC4);
				generate_dword(target, op->result * 4);
			}
			break;

		case B_PUSH:
			{
				generate_byte(target, 0xFF);
				generate_byte(target, 0x75);

				char index = -(op->result  * 4);
				generate_byte(target, index);
			}
			break;

		case B_PUSH_IMM:
			{
				generate_byte(target, 0x68);
				generate_dword(target, op->result);
			}
			break;

		case B_LOAD:
			{
				generate_byte(target, 0x8B);
				generate_byte(target, 0x45);

				char index = -(op->result  * 4);
				generate_byte(target, index);
			}
			break;

		case B_STORE:
			{
				generate_byte(target, 0x89);
				generate_byte(target, 0x45);

				char index = -(op->result  * 4);
				generate_byte(target, index);
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

				char index = -(op->result  * 4);
				generate_byte(target, index);

				// and eax, -5

				generate_byte(target, 0x25);
				generate_dword(target, -5);
			}
			break;

		case B_PHI:
		case B_MOV:
			{
				// Load to eax
				{
					generate_byte(target, 0x8B);
					generate_byte(target, 0x45);

					char index = -(op->left  * 4);
					generate_byte(target, index);
				}

				// Store from eax
				{
					generate_byte(target, 0x89);
					generate_byte(target, 0x45);

					char index = -(op->result  * 4);
					generate_byte(target, index);
				}
			}
			break;

		case B_PHI_IMM:
		case B_MOV_IMM:
			{
				generate_byte(target, 0xC7);
				generate_byte(target, 0x45);

				char index = -(op->result  * 4);
				generate_byte(target, index);

				generate_dword(target, op->left);
			}
			break;

		default:
			break;
	}
}

compiled_block_t compile_block(block_t *block)
{
	unsigned int block_size = 3;

	printf("Compiling block %x:\n", block);

	if (block->var_count - 1 != 0)
		block_size += 6;

	for (size_t i = 0; i < kv_size(block->vector); i++)
		block_size += instruction_size(block, kv_A(block->vector, i), i, block_size);

	block_size += 4;

	compiled_block_t result = code_heap_alloc(block_size);

	unsigned char *target = (unsigned char *)result;

	generate_byte(&target, 0x55);
	generate_byte(&target, 0x89);
	generate_byte(&target, 0xE5);

	if (block->var_count - 1 != 0)
	{
		generate_byte(&target, 0x81);
		generate_byte(&target, 0xEC);
		generate_dword(&target, (block->var_count - 1) * 4);
	}

	for (size_t i = 0; i < kv_size(block->vector); i++)
		generate_instruction(block, kv_A(block->vector, i), i, (unsigned char *)result, &target);

	generate_byte(&target, 0x89);
	generate_byte(&target, 0xEC);
	generate_byte(&target, 0x5D);
	generate_byte(&target, 0xC3);

	dump_code((void *)result, target - (unsigned char *)result);

	assert((unsigned char *)result + block_size == target);

	printf("End compiling block %x:\n", block);

	return result;
}
