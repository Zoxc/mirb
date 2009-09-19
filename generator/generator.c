#include "generator.h"
#include "../runtime/classes.h"
#include "../runtime/classes/symbol.h"
#include "../runtime/classes/fixnum.h"

typedef void(*generator)(block_t *block, struct node *node, variable_t *var);

static inline void gen_node(block_t *block, struct node *node, variable_t *var);

static void gen_unary_op(block_t *block, struct node *node, variable_t *var)
{
	variable_t *temp = var ? var : block_get_var(block);

	gen_node(block, node->left, temp);

	variable_t *args = block_gen_args(block);
	block_end_args(block, args, 0);

	block_push(block, B_CALL, (rt_value)temp, rt_symbol_from_cstr(token_type_names[node->op]), 0);

	if (var)
		block_push(block, B_STORE, (rt_value)var, 0, 0);
}

static void gen_binary_op(block_t *block, struct node *node, variable_t *var)
{
	variable_t *temp1 = var ? var : block_get_var(block);
	variable_t *temp2 = block_get_var(block);

	gen_node(block, node->left, temp1);

	variable_t *args = block_gen_args(block);

	gen_node(block, node->right, temp2);
	block_push(block, B_PUSH, (rt_value)temp2, 0, 0);

	block_end_args(block, args, 1);
	block_push(block, B_CALL, (rt_value)temp1, rt_symbol_from_cstr(token_type_names[node->op]), 0);

	if (var)
		block_push(block, B_STORE, (rt_value)var, 0, 0);
}

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
	variable_t *temp = var ? var : block_get_var(block);

	if(node->left)
		gen_node(block, node->left, temp);

	block_push(block, B_STRING, (rt_value)temp, (rt_value)node->middle, 0);
	block_push(block, B_PUSH, (rt_value)temp, 0, 0);

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
	variable_t *args = block_gen_args(block);

	variable_t *temp = var ? var : block_get_var(block);

	gen_node(block, node->left, temp);

	block_push(block, B_STRING, (rt_value)temp, (rt_value)node->right, 0);

	block_push(block, B_PUSH, (rt_value)temp, 0, 0);

	block_end_args(block, args, gen_string_arg_count(node->left) + 1);

	block_push(block, B_INTERPOLATE, (rt_value)var, 0, 0);
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

static void gen_if(block_t *block, struct node *node, variable_t *var)
{
	variable_t *temp = var ? var : block_get_var(block);
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
	variable_t *temp = var ? var : block_get_var(block);

	gen_node(block, node->left, temp);

	block_push(block, B_PUSH, (rt_value)temp, 0, 0);

	if(node->right)
		return gen_argument(block, node->right, temp) + 1;
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

static void generate_call(block_t *block, struct node *self, rt_value name, struct node *arguments, struct node *block_node, variable_t *var)
{
	variable_t *self_var = block_get_var(block);

	gen_node(block, self, self_var);

	int parameters = 0;

	variable_t *args = block_gen_args(block);

	variable_t *temp = var ? var : block_get_var(block);

	if(arguments)
		parameters = gen_argument(block, arguments, temp);

	variable_t* block_var = 0;

	if(block_node)
	{
		block_var = block_get_var(block);
		block_t *block_attach = gen_block(block_node);

		khiter_t k;
		khash_t(scope) *hash = block_attach->scope->variables;
		size_t upvals = 0;
		variable_t *closure_args = block_gen_args(block);

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

		block_end_args(block, closure_args, upvals);
		block_push(block, B_CLOSURE, (rt_value)block_var, (rt_value)block_attach, 0);
	}

	block_end_args(block, args, parameters);
	block_push(block, B_CALL, (rt_value)self_var, name, (rt_value)block_var);

	if (var)
		block_push(block, B_STORE, (rt_value)var, 0, 0);
}

static void gen_call(block_t *block, struct node *node, variable_t *var)
{
	generate_call(block, node->left, (rt_value)node->middle, node->right->left, node->right->right, var);
}

static void gen_array_call(block_t *block, struct node *node, variable_t *var)
{
	generate_call(block, node->left, rt_symbol_from_cstr("[]"), node->middle, node->right, var);
}

static size_t gen_array_element(block_t *block, struct node *node, variable_t *var)
{
	gen_node(block, node->left, var);

	block_push(block, B_PUSH, (rt_value)var, 0, 0);

	if(node->right)
		return 1 + gen_array_element(block, node->right, var);
	else
		return 1;
}

static void gen_array(block_t *block, struct node *node, variable_t *var)
{
	variable_t *args = block_gen_args(block);

	size_t elements = node->left ? gen_array_element(block, node->left, var ? var : block_get_var(block)) : 0;

	block_end_args(block, args, elements);

	block_push(block, B_ARRAY, (rt_value)var, 0, 0);
}

static void gen_boolean(block_t *block, struct node *node, variable_t *var)
{
	variable_t *temp = var ? var : block_get_var(block);

	gen_node(block, node->left, temp);

	rt_value label_end = block_get_label(block);

	block_push(block, B_TEST, (rt_value)temp, 0, 0);
	block_push(block, (node->op == T_OR || node->op == T_OR_BOOLEAN) ? B_JMPT : B_JMPF, label_end, 0, 0);

	gen_node(block, node->right, var);

	block_emmit_label(block, label_end);
}

static void gen_not(block_t *block, struct node *node, variable_t *var)
{
	if(var)
	{
		variable_t *temp = var ? var : block_get_var(block);

		gen_node(block, node->left, temp);

		rt_value label_true = block_get_label(block);
		rt_value label_end = block_get_label(block);

		block_push(block, B_TEST, (rt_value)temp, 0, 0);

		block_push(block, B_JMPT, label_true, 0, 0);

		block_push(block, B_MOV_IMM, (rt_value)var, RT_TRUE, 0);
		block_push(block, B_JMP, (rt_value)label_end, 0, 0);

		block_emmit_label(block, label_true);
		block_push(block, B_MOV_IMM, (rt_value)var, RT_FALSE, 0);

		block_emmit_label(block, label_end);
	}
	else
		gen_node(block, node->left, 0);
}

static void gen_warn(block_t *block, struct node *node, variable_t *var)
{
	printf("node %d entered in code generation\n", node->type);
}

static void gen_ivar(block_t *block, struct node *node, variable_t *var)
{
	if(var)
	{
		block->self_ref++;
		block_push(block, B_GET_IVAR, (rt_value)var, (rt_value)node->left, 0);
	}
}

static void gen_ivar_assign(block_t *block, struct node *node, variable_t *var)
{
	variable_t *temp = var ? var : block_get_var(block);

	gen_node(block, node->right, temp);

	block->self_ref++;

	block_push(block, B_SET_IVAR, (rt_value)node->left, (rt_value)temp, 0);
}

static void gen_no_equality(block_t *block, struct node *node, variable_t *var)
{
	if(var)
	{
		variable_t *temp1 = var;
		variable_t *temp2 = block_get_var(block);

		gen_node(block, node->left, temp1);
		gen_node(block, node->right, temp2);

		rt_value label_true = block_get_label(block);
		rt_value label_end = block_get_label(block);

		block_push(block, B_CMP, (rt_value)temp1, (rt_value)temp2, 0);

		block_push(block, B_JMPE, label_true, 0, 0);

		block_push(block, B_MOV_IMM, (rt_value)var, RT_TRUE, 0);
		block_push(block, B_JMP, (rt_value)label_end, 0, 0);

		block_emmit_label(block, label_true);
		block_push(block, B_MOV_IMM, (rt_value)var, RT_FALSE, 0);

		block_emmit_label(block, label_end);
	}
	else
	{
		gen_node(block, node->left, 0);
		gen_node(block, node->right, 0);
	}
}

generator generators[] = {gen_unary_op, gen_binary_op, gen_num, gen_var, gen_ivar, gen_ivar_assign, gen_string, gen_string_start, gen_string_continue, gen_array, /*N_ARRAY_ELEMENT*/gen_warn, gen_const, gen_self, gen_true, gen_false, gen_nil, gen_assign, gen_const_assign, gen_boolean, gen_not, gen_no_equality, gen_if, gen_if, /*N_ARGUMENT*/gen_warn, /*N_CALL_ARGUMENTS*/gen_warn, gen_call, gen_array_call, gen_expressions, gen_class, gen_module, /*N_SCOPE*/gen_warn, gen_method};

static inline void gen_node(block_t *block, struct node *node, variable_t *var)
{
	assert(node != 0);

	generators[node->type](block, node, var);
}

block_t *gen_block(struct node *node)
{
	block_t *block = block_create((void *)node->left);

	variable_t *result = block_get_var(block);

	gen_node(block, node->right, result);

	for(int i = 0; i < kv_size(block->upvals); i++)
		block_push(block, B_SEAL, (rt_value)kv_A(block->upvals, i), 0, 0);

	block_push(block, B_LOAD, (rt_value)result, 0, 0);

	printf(";\n; block %x\n;\n", (rt_value)block);

	block_print(block);

	printf("\n\n");

	return block;
}
