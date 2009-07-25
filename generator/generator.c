#include "generator.h"
#include "../runtime/symbols.h"
#include "../runtime/classes.h"

typedef void(*generator)(block_t *block, struct node *node, rt_value var);

static inline void gen_node(block_t *block, struct node *node, rt_value var);

static void gen_num(block_t *block, struct node *node, rt_value var)
{
	if (var)
		block_push(block, B_MOV_IMM, var, RT_INT2FIX(node->left), 0);
}

static void gen_var(block_t *block, struct node *node, rt_value var)
{
	if (var)
		block_push(block, B_MOV, var, (rt_value)node->left, 0);
}

static void gen_assign(block_t *block, struct node *node, rt_value var)
{
	gen_node(block, node->right, (rt_value)node->left);

	if (var)
		block_push(block, B_MOV, var, (rt_value)node->left, 0);
}

static void gen_arithmetic(block_t *block, struct node *node, rt_value var)
{
	rt_value temp1 = block_get_var(block);
	rt_value temp2 = block_get_var(block);

	gen_node(block, node->left, temp1);
	gen_node(block, node->right, temp2);

	block_use_var(block, temp2, block_push(block, B_PUSH, temp2, 0, 0));
	block_push(block, B_PUSH_IMM, 2, 0, 0);
	block_use_var(block, temp1, block_push(block, B_PUSH, temp1, 0, 0));
	block_push(block, B_PUSH_IMM, (rt_value)symbol_get(token_type_names[node->op]), 0, 0);
	block_push(block, B_CALL, 4, 0, 0);

	if (var)
		block_push(block, B_STORE, var, 1, 0);
}

static void gen_if(block_t *block, struct node *node, rt_value var)
{
	rt_value temp = block_get_var(block);
	rt_value label_else = block_get_label(block);

	gen_node(block, node->left, temp);

	block_use_var(block, temp, block_push(block, B_TEST, temp, 0, 0));

	block_push(block, node->type == N_IF ? B_JMPF : B_JMPT, label_else, 0, 0);

	rt_value label_end = block_get_label(block);

	if(var)
	{
		rt_value result_left = block_get_var(block);
		rt_value result_right = block_get_var(block);

		gen_node(block, node->middle, result_left);
		block_use_var(block, result_left, block_push(block, B_PHI, var, result_left, 0));
		block_push(block, B_JMP, label_end, 0, 0);

		block_emmit_label(block, label_else);

		gen_node(block, node->right, result_right);
		block_use_var(block, result_right, block_push(block, B_PHI, var, result_right, 0));

		block_emmit_label(block, label_end);
	}
	else
	{
		gen_node(block, node->middle, 0);
		block_push(block, B_JMP, label_end, 0, 0);

		block_emmit_label(block, label_else);

		gen_node(block, node->right, 0);

		block_emmit_label(block, label_end);
	}
}

static void gen_nil(block_t *block, struct node *node, rt_value var)
{
	if(var)
		block_push(block, B_MOV_IMM, var, RT_NIL, 0);
}

static void gen_expressions(block_t *block, struct node *node, rt_value var)
{
	gen_node(block, node->left, 0);

	if(node->right)
		gen_node(block, node->right, var);
}

static void gen_class(block_t *block, struct node *node, rt_value var)
{
	if (block->scope->type == S_MAIN)
		block_push(block, B_DEF_CLASS_MAIN, var, (rt_value)node->left, (rt_value)gen_block(node->right));
	else
		block_push(block, B_DEF_CLASS, var, (rt_value)node->left, (rt_value)gen_block(node->right));
}

static void gen_warn(block_t *block, struct node *node, rt_value var)
{
	printf("node %d entered in code generation\n", node->type);
}

generator generators[] = {gen_num, gen_var, gen_assign, gen_arithmetic, gen_arithmetic, gen_if, gen_if, gen_nil, /*name_argument*/gen_warn, /*name_message*/gen_warn, /*name_array_message*/gen_warn, /*name_call_tail*/gen_warn, /*name_call*/gen_warn, gen_expressions, gen_class, /*N_SCOPE*/gen_warn};

static inline void gen_node(block_t *block, struct node *node, rt_value var)
{
	generators[node->type](block, node, var);
}

block_t *gen_block(struct node *node)
{
	block_t *block = block_create((void *)node->left);

	printf("Generating block %x:\n", block);

	rt_value result = block_get_var(block);

	gen_node(block, node->right, result);

	block_push(block, B_LOAD, result, 0, 0);

	block_print(block);

	//block_optimize(block);

	//printf("Optimized block %x:\n", block);

	//block_print(block);

	printf("End generating block %x:\n", block);

	return block;
}
