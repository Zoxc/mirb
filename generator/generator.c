#include "generator.h"
#include "../runtime/symbol.h"
#include "../runtime/classes.h"
#include "../runtime/fixnum.h"

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

static void gen_string(block_t *block, struct node *node, rt_value var)
{
	if (var)
		block_push(block, B_STRING, var, (rt_value)node->left, 0);
}

static void gen_string_continue(block_t *block, struct node *node, rt_value var)
{
	if(node->left)
		gen_node(block, node->left, 0);

	rt_value string = block_get_var(block);

	block_push(block, B_STRING, string, (rt_value)node->middle, 0);

	block_use_var(block, string, block_push(block, B_PUSH, string, 0, 0));


	rt_value interpolated = block_get_var(block);

	gen_node(block, node->right, interpolated);

	block_use_var(block, interpolated, block_push(block, B_PUSH, interpolated, 0, 0));
}

static unsigned int gen_string_arg_count(struct node *node)
{
	if(node->left)
		return 2 + gen_string_arg_count(node->left);
	else
		return 2;
}

static void gen_string_start(block_t *block, struct node *node, rt_value var)
{
	gen_node(block, node->left, 0);


	rt_value string = block_get_var(block);

	block_push(block, B_STRING, string, (rt_value)node->right, 0);

	block_use_var(block, string, block_push(block, B_PUSH, string, 0, 0));

	unsigned int argc = gen_string_arg_count(node->left) + 1;

	block_push(block, B_PUSH_IMM, argc, 0, 0);
	block_push(block, B_INTERPOLATE, var, argc + 1, 0);
}


static void gen_const(block_t *block, struct node *node, rt_value var)
{
	if (var)
	{
		rt_value self = block_get_var(block);

		gen_node(block, node->left, self);

		block_use_var(block, self, block_push(block, B_GET_CONST, var, self, (rt_value)node->right));
	}
}

static void gen_self(block_t *block, struct node *node, rt_value var)
{
	if (var)
	{
		block->self_ref++;
		block_push(block, B_SELF, var, 0, 0);
	}
}

static void gen_true(block_t *block, struct node *node, rt_value var)
{
	if (var)
		block_push(block, B_MOV_IMM, var, RT_TRUE, 0);
}

static void gen_false(block_t *block, struct node *node, rt_value var)
{
	if (var)
		block_push(block, B_MOV_IMM, var, RT_FALSE, 0);
}

static void gen_nil(block_t *block, struct node *node, rt_value var)
{
	if (var)
		block_push(block, B_MOV_IMM, var, RT_NIL, 0);
}

static void gen_assign(block_t *block, struct node *node, rt_value var)
{
	gen_node(block, node->right, (rt_value)node->left);

	if (var)
		block_push(block, B_MOV, var, (rt_value)node->left, 0);
}

static void gen_const_assign(block_t *block, struct node *node, rt_value var)
{
	rt_value self = block_get_var(block);
	rt_value value = block_get_var(block);

	gen_node(block, node->left, self);

	gen_node(block, node->right, value);

	block_use_var(block, self, block_push(block, B_SET_CONST, self, (rt_value)node->middle, value));

	if (var)
		block_push(block, B_MOV, var, value, 0);
}

static void gen_unary(block_t *block, struct node *node, rt_value var)
{
	printf("generating unary %s\n", token_type_names[node->op]);
	gen_node(block, node->left, var);
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
	block_push(block, B_PUSH_IMM, (rt_value)rt_symbol_from_cstr(token_type_names[node->op]), 0, 0);
	block_push(block, B_CALL, 4, 0, 0);

	if (var)
		block_push(block, B_STORE, var, 0, 0);
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
		block_use_var(block, result_left, block_push(block, B_MOV, var, result_left, 0));
		block_push(block, B_JMP, label_end, 0, 0);

		block_emmit_label(block, label_else);

		gen_node(block, node->right, result_right);
		block_use_var(block, result_right, block_push(block, B_MOV, var, result_right, 0));

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

static void gen_expressions(block_t *block, struct node *node, rt_value var)
{
	gen_node(block, node->left, 0);

	if (node->right)
		gen_node(block, node->right, var);
}

static void gen_class(block_t *block, struct node *node, rt_value var)
{
	block->self_ref++;
	block_push(block, B_CLASS, (rt_value)node->left, (rt_value)gen_block(node->right), 0);

	if (var)
		block_push(block, B_STORE, var, 0, 0);
}

static void gen_method(block_t *block, struct node *node, rt_value var)
{
	block->self_ref++;
	block_push(block, B_METHOD, (rt_value)node->left, (rt_value)gen_block(node->right), 0);

	if (var)
		block_push(block, B_MOV_IMM, var, RT_NIL, 0);
}

static int gen_argument(block_t *block, struct node *node, rt_value var)
{
	rt_value temp = block_get_var(block);

	gen_node(block, node->left, temp);

	block_use_var(block, temp, block_push(block, B_PUSH, temp, 0, 0));

	if(node->right)
		return gen_argument(block, node->right, 0) + 1;
	else
		return 1;
}

static void gen_call(block_t *block, struct node *node, rt_value var)
{
	rt_value self = block_get_var(block);

	gen_node(block, node->left, self);

	int parameters = 0;

	if((rt_value)node->right > 1)
		parameters = gen_argument(block, node->right, 0);

	block_push(block, B_PUSH_IMM, parameters, 0, 0);
	block_use_var(block, self, block_push(block, B_PUSH, self, 0, 0));
	block_push(block, B_PUSH_IMM, (rt_value)node->middle, 0, 0);
	block_push(block, B_CALL, 2 + parameters, 0, 0);

	if (var)
		block_push(block, B_STORE, var, 0, 0);
}

static void gen_warn(block_t *block, struct node *node, rt_value var)
{
	printf("node %d entered in code generation\n", node->type);
}

generator generators[] = {gen_num, gen_var, gen_string, gen_string_start, gen_string_continue, gen_const, gen_self, gen_true, gen_false, gen_nil, gen_assign, gen_const_assign, gen_unary, gen_arithmetic, gen_arithmetic, gen_if, gen_if, (generator)gen_argument, gen_call, /*N_ASSIGN_MESSAGE*/gen_warn, gen_expressions, gen_class, /*N_SCOPE*/gen_warn, gen_method};

static inline void gen_node(block_t *block, struct node *node, rt_value var)
{
	assert(node != 0);

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

	printf("End generating block %x:\n", block);

	return block;
}
