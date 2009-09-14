#include "generator.h"
#include "../runtime/classes.h"
#include "../runtime/classes/symbol.h"
#include "../runtime/classes/fixnum.h"

typedef void(*generator)(block_t *block, struct node *node, variable_t *var);

static inline void gen_node(block_t *block, struct node *node, variable_t *var);

static void gen_num(block_t *block, struct node *node, variable_t *var)
{
	if (var)
		block_push(block, B_MOV_IMM, (rt_value)var, RT_INT2FIX(node->left), 0);
}

static void gen_var(block_t *block, struct node *node, variable_t *var)
{
	if(var)
	{
		variable_t *reading = (variable_t *)node->left;

		if(reading->type == V_UPVAL)
			block_push(block, B_GET_UPVAL, (rt_value)var, (rt_value)reading, 0);
		else
			block_push(block, B_MOV, (rt_value)var, (rt_value)reading, 0);
	}
}

static void gen_string(block_t *block, struct node *node, variable_t *var)
{
	if (var)
		block_push(block, B_STRING, (rt_value)var, (rt_value)node->left, 0);
}

static void gen_string_continue(block_t *block, struct node *node, variable_t *var)
{
	if(node->left)
		gen_node(block, node->left, 0);

	variable_t *string = block_get_var(block);

	block_push(block, B_STRING, (rt_value)string, (rt_value)node->middle, 0);
	block_push(block, B_PUSH, (rt_value)string, 0, 0);

	variable_t *interpolated = block_get_var(block);

	gen_node(block, node->right, interpolated);

	block_push(block, B_PUSH, (rt_value)interpolated, 0, 0);
}

static size_t gen_string_arg_count(struct node *node)
{
	if(node->left)
		return 2 + gen_string_arg_count(node->left);
	else
		return 2;
}

static void gen_string_start(block_t *block, struct node *node, variable_t *var)
{
	gen_node(block, node->left, 0);


	variable_t *string = block_get_var(block);

	block_push(block, B_STRING, (rt_value)string, (rt_value)node->right, 0);
	block_push(block, B_PUSH, (rt_value)string, 0, 0);

	size_t argc = gen_string_arg_count(node->left) + 1;

	block_push(block, B_PUSH_IMM, argc, 0, 0);
	block_push(block, B_INTERPOLATE, (rt_value)var, argc + 1, 0);
}


static void gen_const(block_t *block, struct node *node, variable_t *var)
{
	if (var)
	{
		variable_t *self = block_get_var(block);

		gen_node(block, node->left, self);

		block_push(block, B_GET_CONST, (rt_value)var, (rt_value)self, (rt_value)node->right);
	}
}

static void gen_self(block_t *block, struct node *node, variable_t *var)
{
	if (var)
	{
		block->self_ref++;
		block_push(block, B_SELF, (rt_value)var, 0, 0);
	}
}

static void gen_true(block_t *block, struct node *node, variable_t *var)
{
	if (var)
		block_push(block, B_MOV_IMM, (rt_value)var, RT_TRUE, 0);
}

static void gen_false(block_t *block, struct node *node, variable_t *var)
{
	if (var)
		block_push(block, B_MOV_IMM, (rt_value)var, RT_FALSE, 0);
}

static void gen_nil(block_t *block, struct node *node, variable_t *var)
{
	if (var)
		block_push(block, B_MOV_IMM, (rt_value)var, RT_NIL, 0);
}

static void gen_assign(block_t *block, struct node *node, variable_t *var)
{
	variable_t *reading = (variable_t *)node->left;

	if(reading->type == V_UPVAL)
	{
		variable_t *temp = block_get_var(block);

		gen_node(block, node->right, temp);

		block_push(block, B_SET_UPVAL, (rt_value)reading, (rt_value)temp, 0);

		if (var)
			block_push(block, B_MOV, (rt_value)var, (rt_value)temp, 0);
	}
	else
	{
		gen_node(block, node->right, (variable_t *)node->left);

		if (var)
			block_push(block, B_MOV, (rt_value)var, (rt_value)node->left, 0);
	}
}

static void gen_const_assign(block_t *block, struct node *node, variable_t *var)
{
	variable_t *self = block_get_var(block);
	variable_t *value = block_get_var(block);

	gen_node(block, node->left, self);

	gen_node(block, node->right, value);

	block_push(block, B_SET_CONST, (rt_value)self, (rt_value)node->middle, (rt_value)value);

	if (var)
		block_push(block, B_MOV, (rt_value)var, (rt_value)value, 0);
}

static void gen_unary(block_t *block, struct node *node, variable_t *var)
{
	printf("generating unary %s\n", token_type_names[node->op]);
	gen_node(block, node->left, var);
}

static void gen_arithmetic(block_t *block, struct node *node, variable_t *var)
{
	variable_t *temp1 = block_get_var(block);
	variable_t *temp2 = block_get_var(block);

	gen_node(block, node->left, temp1);
	gen_node(block, node->right, temp2);

	block_push(block, B_PUSH, (rt_value)temp2, 0, 0);
	block_push(block, B_ARGS, 1, 0, 0);
	block_push(block, B_PUSH_RAW, 1, 0, 0);
	block_push(block, B_PUSH_IMM, RT_NIL, 0, 0);
	block_push(block, B_PUSH, (rt_value)temp1, 0, 0);
	block_push(block, B_CALL, rt_symbol_from_cstr(token_type_names[node->op]), 1, 0);

	if (var)
		block_push(block, B_STORE, (rt_value)var, 0, 0);
}

static void gen_if(block_t *block, struct node *node, variable_t *var)
{
	variable_t *temp = block_get_var(block);
	rt_value label_else = block_get_label(block);

	gen_node(block, node->left, temp);

	block_push(block, B_TEST, (rt_value)temp, 0, 0);
	block_push(block, node->type == N_IF ? B_JMPF : B_JMPT, label_else, 0, 0);

	rt_value label_end = block_get_label(block);

	if(var)
	{
		variable_t *result_left = block_get_var(block);
		variable_t *result_right = block_get_var(block);

		gen_node(block, node->middle, result_left);
		block_push(block, B_MOV, (rt_value)var, (rt_value)result_left, 0);
		block_push(block, B_JMP, label_end, 0, 0);

		block_emmit_label(block, label_else);

		gen_node(block, node->right, result_right);
		block_push(block, B_MOV, (rt_value)var, (rt_value)result_right, 0);

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

static void gen_expressions(block_t *block, struct node *node, variable_t *var)
{
	gen_node(block, node->left, 0);

	if (node->right)
		gen_node(block, node->right, var);
}

static void gen_class(block_t *block, struct node *node, variable_t *var)
{
	block->self_ref++;
	block_push(block, B_CLASS, (rt_value)node->left, (rt_value)gen_block(node->right), 0);

	if (var)
		block_push(block, B_STORE, (rt_value)var, 0, 0);
}

static void gen_module(block_t *block, struct node *node, variable_t *var)
{
	block->self_ref++;
	block_push(block, B_MODULE, (rt_value)node->left, (rt_value)gen_block(node->right), 0);

	if (var)
		block_push(block, B_STORE, (rt_value)var, 0, 0);
}

static void gen_method(block_t *block, struct node *node, variable_t *var)
{
	block->self_ref++;
	block_push(block, B_METHOD, (rt_value)node->left, (rt_value)gen_block(node->right), 0);

	if (var)
		block_push(block, B_MOV_IMM, (rt_value)var, RT_NIL, 0);
}

static int gen_argument(block_t *block, struct node *node, variable_t *var)
{
	variable_t *temp = block_get_var(block);

	gen_node(block, node->left, temp);

	block_push(block, B_PUSH, (rt_value)temp, 0, 0);

	if(node->right)
		return gen_argument(block, node->right, 0) + 1;
	else
		return 1;
}

static variable_t *get_upval(block_t *block, variable_t *var)
{
	if(var->real)
	{
		return var->real;
	}
	else
	{
		variable_t *temp = block_get_var(block);

		var->real = temp;
		var->index = block->scope->var_count[V_UPVAL];

		block->scope->var_count[V_UPVAL] += 1;

		block_push(block, B_UPVAL, (rt_value)temp, (rt_value)var, 0);

		kv_push(variable_t *, block->upvals, temp);

		return temp;
	}
}

static void gen_call(block_t *block, struct node *node, variable_t *var)
{
	variable_t *self = block_get_var(block);

	gen_node(block, node->left, self);

	int parameters = 0;

	if(node->right->left)
		parameters = gen_argument(block, node->right->left, 0);

	variable_t* block_var = 0;

	if(node->right->right)
	{
		block_var = block_get_var(block);
		block_t *block_attach = gen_block(node->right->right);

		khiter_t k;
		khash_t(scope) *hash = block_attach->scope->variables;
		size_t upvals = 0;

		for (k = kh_begin(hash); k != kh_end(hash); ++k)
		{
			if(kh_exist(hash, k))
			{
				variable_t *var = kh_value(hash, k);

				if(var->type == V_UPVAL)
				{
					khiter_t k = kh_get(scope, block->scope->variables, var->name);

					if(k != kh_end(block->scope->variables) && kh_value(block->scope->variables, k)->real == var->real)
						block_push(block, B_PUSH_UPVAL, (rt_value)kh_value(block->scope->variables, k), 0, 0);
					else
						block_push(block, B_PUSH, (rt_value)get_upval(block, var->real), 0, 0);

					upvals++;
				}
			}
		}

		block_push(block, B_CLOSURE, (rt_value)block_var, (rt_value)block_attach, upvals);
	}

	block_push(block, B_ARGS, 1, 0, 0);
	block_push(block, B_PUSH_RAW, parameters, 0, 0);

	if(block_var)
		block_push(block, B_PUSH, (rt_value)block_var, 0, 0);
	else
		block_push(block, B_PUSH_IMM, RT_NIL, 0, 0);

	block_push(block, B_PUSH, (rt_value)self, 0, 0);
	block_push(block, B_CALL, (rt_value)node->middle, parameters, (rt_value)block_var);

	if (var)
		block_push(block, B_STORE, (rt_value)var, 0, 0);
}

static void gen_warn(block_t *block, struct node *node, variable_t *var)
{
	printf("node %d entered in code generation\n", node->type);
}

generator generators[] = {gen_num, gen_var, gen_string, gen_string_start, gen_string_continue, gen_const, gen_self, gen_true, gen_false, gen_nil, gen_assign, gen_const_assign, gen_unary, gen_arithmetic, gen_arithmetic, gen_if, gen_if, (generator)gen_argument, /*N_CALL_ARGUMENTS*/gen_warn, gen_call, gen_expressions, gen_class, gen_module, /*N_SCOPE*/gen_warn, gen_method};

static inline void gen_node(block_t *block, struct node *node, variable_t *var)
{
	assert(node != 0);

	generators[node->type](block, node, var);
}

block_t *gen_block(struct node *node)
{
	block_t *block = block_create((void *)node->left);

	printf("Generating block %x:\n", (rt_value)block);

	variable_t *result = block_get_var(block);

	gen_node(block, node->right, result);

	for(int i = 0; i < kv_size(block->upvals); i++)
		block_push(block, B_SEAL, (rt_value)kv_A(block->upvals, i), 0, 0);

	block_push(block, B_LOAD, (rt_value)result, 0, 0);

	block_print(block);

	printf("End generating block %x:\n", (rt_value)block);

	return block;
}
