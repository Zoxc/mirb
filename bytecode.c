#include "bytecode.h"

void opcode_print(opcode_t *op)
{
	switch(op->type)
	{
		case B_NOP:
			printf("nop\n", op->result);
			break;

		case B_MOV_NUM:
			printf("mov "); block_print_var(op->result); printf(", %d\n", op->left);
			break;

		case B_MOV:
			printf("mov "); block_print_var(op->result); printf(", "); block_print_var(op->left);  printf("\n");
			break;

		case B_PUSH:
		case B_PUSH_OBJECT:
			printf("push "); block_print_var(op->result); printf("\n");
			break;

		case B_PUSH_NUM:
			printf("push %d\n", op->result);
			break;

		case B_CALL:
			printf("call\n", op->result);
			break;

		case B_TEST:
			printf("test "); block_print_var(op->result); printf("\n");
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
			printf("#%d:\n", (op->result - 1) / 2);
			break;

		default:
			printf("unknown opcode %d\n", op->type);
	}
}

static inline bool is_temp_var(OP_VAR var)
{
	return (var & 1 && var != 1);
}

void block_optimize(block_t *block)
{
	size_t count = kv_size(block->vector);
	for(size_t i = 0; i < count; i++)
	{
		opcode_t *op = kv_A(block->vector, i);

		switch(op->type)
		{
			case B_MOV:
				if(is_temp_var(op->result) && (i + 1) < count)
				{
					opcode_t *target = kv_A(block->vector, i + 1);

					switch(target->type)
					{
						case B_PUSH:
							target->left = op->left;
							op->type = B_NOP;
							break;

						default:
							break;
					}
				}
				break;

			case B_MOV_NUM:
				if(is_temp_var(op->result))
				{
					khiter_t k = kh_get(OP_VAR, (block->var_usage), op->result);

					if (k != kh_end((block->var_usage)))
					{
						opcode_t *target = kv_A(block->vector, kh_value((block->var_usage), k));

						switch(target->type)
						{
							case B_PUSH:
								target->type = B_PUSH_NUM;
								target->result = op->left;
								op->type = B_NOP;
								break;

							default:
								break;
						}
					}
				}
				break;

			default:
				break;
		}
	}
}

void block_print(block_t *block)
{
	for(size_t i = 0; i < kv_size(block->vector); i++)
		opcode_print(kv_A(block->vector, i));
}
