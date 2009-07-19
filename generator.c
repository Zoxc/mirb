#include "generator.h"
#include "symbols.h"

typedef void(*generator)(block_t *block, struct node *node, OP_VAR var);

static inline void gen_node(block_t *block, struct node *node, OP_VAR var);

static void gen_num(block_t *block, struct node *node, OP_VAR var)
{
	if (var)
		block_push(block, B_MOV_IMM, var, (OP_VAR)node->right, 0);
}

static void gen_var(block_t *block, struct node *node, OP_VAR var)
{
	if (var)
		block_push(block, B_MOV, var, (OP_VAR)node->left, 0);
}

static void gen_assign(block_t *block, struct node *node, OP_VAR var)
{
	gen_node(block, node->right, (OP_VAR)node->left);

	if (var)
		block_push(block, B_MOV, var, (OP_VAR)node->left, 0);
}

static void gen_arithmetic(block_t *block, struct node *node, OP_VAR var)
{
	OP_VAR temp1 = block_get_var(block);
	OP_VAR temp2 = block_get_var(block);

	gen_node(block, node->left, temp1);
	gen_node(block, node->right, temp2);

	block_use_var(block, temp2, block_push(block, B_PUSH, temp2, 0, 0));
	block_push(block, B_PUSH_IMM, 4, 0, 0);
	block_push(block, B_PUSH_OBJECT, (OP_VAR)symbol_get(token_type_names[node->op]), 0, 0);
	block_use_var(block, temp1, block_push(block, B_PUSH, temp1, 0, 0));
	block_push(block, B_CALL, 0, 0, 0);

	if (var)
		block_push(block, B_MOV, var, 1, 0);
}

static void gen_if(block_t *block, struct node *node, OP_VAR var)
{
	OP_VAR temp = block_get_var(block);
	OP_VAR label_else = block_get_var(block);

	gen_node(block, node->left, temp);

	block_use_var(block, temp, block_push(block, B_TEST, temp, 0, 0));
	block_push(block, B_JMPF, label_else, 0, 0);

	if (node->right->type == N_ELSE)
	{
		OP_VAR label_end = block_get_var(block);

		if(var)
		{
			OP_VAR result_left = block_get_var(block);
			OP_VAR result_right = block_get_var(block);

			gen_node(block, node->right->left, result_left);
			block_use_var(block, result_left, block_push(block, B_PHI, var, result_left, 0));
			block_push(block, B_JMP, label_end, 0, 0);

			block_push(block, B_LABEL, label_else, 0, 0);

			gen_node(block, node->right->right, result_right);
			block_use_var(block, result_right, block_push(block, B_PHI, var, result_right, 0));

			block_push(block, B_LABEL, label_end, 0, 0);
		}
		else
		{
			gen_node(block, node->right->left, 0);
			block_push(block, B_JMP, label_end, 0, 0);
			block_push(block, B_LABEL, label_else, 0, 0);
			gen_node(block, node->right->right, 0);
			block_push(block, B_LABEL, label_end, 0, 0);
		}
	}
	else
	{
		if(var)
		{
			OP_VAR label_end = block_get_var(block);

			OP_VAR result_left = block_get_var(block);

			gen_node(block, node->right->left, result_left);
			block_use_var(block, result_left, block_push(block, B_PHI, var, result_left, 0));
			block_push(block, B_JMP, label_end, 0, 0);

			block_push(block, B_LABEL, label_else, 0, 0);

			block_push(block, B_PHI_IMM, var, (OP_VAR)-1, 0);

			block_push(block, B_LABEL, label_end, 0, 0);
		}
		else
		{
			gen_node(block, node->right, 0);
			block_push(block, B_LABEL, label_else, 0, 0);
		}
	}
}

generator generators[] = {gen_num, gen_var, gen_assign, gen_arithmetic, gen_arithmetic, gen_if, 0/*N_ELSE*/};

static inline void gen_node(block_t *block, struct node *node, OP_VAR var)
{
	generators[node->type](block, node, var);
}

block_t *gen_block(struct node *node)
{
	block_t *block = block_create();

	printf("Generated code:\n");

	gen_node(block, node, 0);

	block_print(block);

	block_optimize(block);

	printf("Optimized code:\n");

	block_print(block);

	return block;
}
