#include "bytecode.h"

void opcode_print(opcode_t *op)
{
	switch(op->type)
	{
		case B_NOP:
			printf("nop\n");
			break;

		case B_MOV_IMM:
			printf("mov "); block_print_var(op->result); printf(", %d\n", op->left);
			break;

		case B_MOV:
			printf("mov "); block_print_var(op->result); printf(", "); block_print_var(op->left);  printf("\n");
			break;

		case B_PUSH:
		case B_PUSH_OBJECT:
			printf("push "); block_print_var(op->result); printf("\n");
			break;

		case B_PUSH_IMM:
			printf("push %d\n", op->result);
			break;

		case B_CALL:
			printf("call\n");
			break;

		case B_TEST:
			printf("test "); block_print_var(op->result); printf("\n");
			break;

		case B_TEST_IMM:
			printf("test %d", op->result); printf("\n");
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

		case B_PHI_IMM:
			printf("phi "); block_print_var(op->result); printf(", %d\n", op->left);
			break;

		case B_PHI:
			printf("phi "); block_print_var(op->result); printf(", "); block_print_var(op->left); printf("\n");
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
					khiter_t k = kh_get(OP_VAR, block->var_usage, op->result);

					if (k == kh_end((block->var_usage)))
						break;

					unsigned int entry = kh_value(block->var_usage, k);;

					if(op->result == 1) // Result register can be overwritten!
					{
						bool safe = true;

						for(unsigned int iter = i + 1; i < entry; i++)
						{
							opcode_t *current_op = kv_A(block->vector, iter);

							if(current_op->type == B_CALL)
							{
								safe = false;
								break;
							}
						}

						if(!safe)
							break;
					}

					opcode_t *target = kv_A(block->vector, entry);

					switch(target->type)
					{
						case B_PUSH:
							target->result = op->left;
							op->type = B_NOP;
							break;

						case B_PHI:
							target->left = op->left;
							op->type = B_NOP;
							break;

						default:
							break;
					}
				}
				break;

			case B_MOV_IMM:
				if(is_temp_var(op->result))
				{
					khiter_t k = kh_get(OP_VAR, block->var_usage, op->result);

					if (k == kh_end((block->var_usage)))
						break;

					opcode_t *target = kv_A(block->vector, kh_value(block->var_usage, k));

					switch(target->type)
					{
						case B_PHI:
							target->type = B_PHI_IMM;
							target->left = op->left;
							op->type = B_NOP;
							break;

						case B_PUSH:
							target->type = B_PUSH_IMM;
							target->result = op->left;
							op->type = B_NOP;
							break;

						case B_TEST:
							target->type = B_TEST_IMM;
							target->result = op->left;
							op->type = B_NOP;
							break;

						default:
							break;
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
	{
		opcode_t *op = kv_A(block->vector, i);

		if(op->type != B_NOP)
			opcode_print(kv_A(block->vector, i));
	}
}
