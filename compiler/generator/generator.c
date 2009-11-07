#include "generator.h"
#include "../../runtime/classes.h"
#include "../../runtime/classes/symbol.h"
#include "../../runtime/classes/fixnum.h"
#include "../../runtime/support.h"

typedef void(*generator)(struct block *block, struct node *node, struct variable *var);

static inline void gen_node(struct block *block, struct node *node, struct variable *var);

static void gen_unary_op(struct block *block, struct node *node, struct variable *var)
{
	struct variable *temp = var ? var : block_get_var(block);

	gen_node(block, node->left, temp);

	struct variable *args = block_gen_args(block);
	block_end_args(block, args, 0);

	block_push(block, B_CALL, (rt_value)temp, rt_symbol_from_cstr(token_type_names[node->op]), 0);

	if (var)
		block_push(block, B_STORE, (rt_value)var, 0, 0);
}

static void gen_binary_op(struct block *block, struct node *node, struct variable *var)
{
	struct variable *temp1 = var ? var : block_get_var(block);
	struct variable *temp2 = block_get_var(block);

	gen_node(block, node->left, temp1);

	struct variable *args = block_gen_args(block);

	gen_node(block, node->right, temp2);
	block_push(block, B_PUSH, (rt_value)temp2, 0, 0);

	block_end_args(block, args, 1);
	block_push(block, B_CALL, (rt_value)temp1, rt_symbol_from_cstr(token_type_names[node->op]), 0);

	if (var)
		block_push(block, B_STORE, (rt_value)var, 0, 0);
}

static void gen_num(struct block *block, struct node *node, struct variable *var)
{
	if (var)
		block_push(block, B_MOV_IMM, (rt_value)var, RT_INT2FIX(node->left), 0);
}

static void gen_var(struct block *block, struct node *node, struct variable *var)
{
	if(var)
	{
		struct variable *reading = (struct variable *)node->left;

		if(reading->type == V_UPVAL)
			block_push(block, B_GET_UPVAL, (rt_value)var, (rt_value)reading, 0);
		else
			block_push(block, B_MOV, (rt_value)var, (rt_value)reading, 0);
	}
}

static void gen_string(struct block *block, struct node *node, struct variable *var)
{
	if (var)
		block_push(block, B_STRING, (rt_value)var, (rt_value)node->left, 0);
}

static void gen_string_continue(struct block *block, struct node *node, struct variable *var)
{
	struct variable *temp = var ? var : block_get_var(block);

	if(node->left)
		gen_node(block, node->left, temp);

	block_push(block, B_STRING, (rt_value)temp, (rt_value)node->middle, 0);
	block_push(block, B_PUSH, (rt_value)temp, 0, 0);

	struct variable *interpolated = block_get_var(block);

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

static void gen_string_start(struct block *block, struct node *node, struct variable *var)
{
	struct variable *args = block_gen_args(block);

	struct variable *temp = var ? var : block_get_var(block);

	gen_node(block, node->left, temp);

	block_push(block, B_STRING, (rt_value)temp, (rt_value)node->right, 0);

	block_push(block, B_PUSH, (rt_value)temp, 0, 0);

	block_end_args(block, args, gen_string_arg_count(node->left) + 1);

	block_push(block, B_INTERPOLATE, (rt_value)var, 0, 0);
}


static void gen_const(struct block *block, struct node *node, struct variable *var)
{
	if (var)
	{
		struct variable *self = block_get_var(block);

		gen_node(block, node->left, self);

		block_push(block, B_GET_CONST, (rt_value)var, (rt_value)self, (rt_value)node->right);
	}
}

static void gen_self(struct block *block, struct node *node, struct variable *var)
{
	if (var)
	{
		block->self_ref++;
		block_push(block, B_SELF, (rt_value)var, 0, 0);
	}
}

static void gen_true(struct block *block, struct node *node, struct variable *var)
{
	if (var)
		block_push(block, B_MOV_IMM, (rt_value)var, RT_TRUE, 0);
}

static void gen_false(struct block *block, struct node *node, struct variable *var)
{
	if (var)
		block_push(block, B_MOV_IMM, (rt_value)var, RT_FALSE, 0);
}

static void gen_nil(struct block *block, struct node *node, struct variable *var)
{
	if (var)
		block_push(block, B_MOV_IMM, (rt_value)var, RT_NIL, 0);
}

static void gen_assign(struct block *block, struct node *node, struct variable *var)
{
	struct variable *reading = (struct variable *)node->left;

	if(reading->type == V_UPVAL)
	{
		struct variable *temp = block_get_var(block);

		gen_node(block, node->right, temp);

		block_push(block, B_SET_UPVAL, (rt_value)reading, (rt_value)temp, 0);

		if (var)
			block_push(block, B_MOV, (rt_value)var, (rt_value)temp, 0);
	}
	else
	{
		gen_node(block, node->right, (struct variable *)node->left);

		if (var)
			block_push(block, B_MOV, (rt_value)var, (rt_value)node->left, 0);
	}
}

static void gen_const_assign(struct block *block, struct node *node, struct variable *var)
{
	struct variable *self = block_get_var(block);
	struct variable *value = block_get_var(block);

	gen_node(block, node->left, self);

	gen_node(block, node->right, value);

	block_push(block, B_SET_CONST, (rt_value)self, (rt_value)node->middle, (rt_value)value);

	if (var)
		block_push(block, B_MOV, (rt_value)var, (rt_value)value, 0);
}

static void gen_if(struct block *block, struct node *node, struct variable *var)
{
	struct variable *temp = var ? var : block_get_var(block);
	opcode_t *label_else = block_get_label(block);

	gen_node(block, node->left, temp);

	block_push(block, B_TEST, (rt_value)temp, 0, 0);
	block_push(block, node->type == N_IF ? B_JMPF : B_JMPT, (rt_value)label_else, 0, 0);

	opcode_t *label_end = block_get_label(block);

	if(var)
	{
		struct variable *result_left = block_get_var(block);
		struct variable *result_right = block_get_var(block);

		gen_node(block, node->middle, result_left);
		block_push(block, B_MOV, (rt_value)var, (rt_value)result_left, 0);
		block_push(block, B_JMP, (rt_value)label_end, 0, 0);

		block_emmit_label(block, label_else);

		gen_node(block, node->right, result_right);
		block_push(block, B_MOV, (rt_value)var, (rt_value)result_right, 0);

		block_emmit_label(block, label_end);
	}
	else
	{
		gen_node(block, node->middle, 0);
		block_push(block, B_JMP, (rt_value)label_end, 0, 0);

		block_emmit_label(block, label_else);

		gen_node(block, node->right, 0);

		block_emmit_label(block, label_end);
	}
}

static void gen_expressions(struct block *block, struct node *node, struct variable *var)
{
	gen_node(block, node->left, 0);

	if (node->right)
		gen_node(block, node->right, var);
}

static void gen_class(struct block *block, struct node *node, struct variable *var)
{
	block->self_ref++;
	block_push(block, B_CLASS, (rt_value)node->left, (rt_value)gen_block(node->right), 0);

	if (var)
		block_push(block, B_STORE, (rt_value)var, 0, 0);
}

static void gen_module(struct block *block, struct node *node, struct variable *var)
{
	block->self_ref++;
	block_push(block, B_MODULE, (rt_value)node->left, (rt_value)gen_block(node->right), 0);

	if (var)
		block_push(block, B_STORE, (rt_value)var, 0, 0);
}

static void gen_method(struct block *block, struct node *node, struct variable *var)
{
	block->self_ref++;
	block_push(block, B_METHOD, (rt_value)node->left, (rt_value)gen_block(node->right), 0);

	if (var)
		block_push(block, B_MOV_IMM, (rt_value)var, RT_NIL, 0);
}

static int gen_argument(struct block *block, struct node *node, struct variable *var)
{
	struct variable *temp = var ? var : block_get_var(block);

	gen_node(block, node->left, temp);

	block_push(block, B_PUSH, (rt_value)temp, 0, 0);

	if(node->right)
		return gen_argument(block, node->right, temp) + 1;
	else
		return 1;
}

static struct variable *get_upval(struct block *block, struct variable *var)
{
	if(var->real)
	{
		return var->real;
	}
	else
	{
		struct variable *temp = block_get_var(block);

		var->real = temp;
		var->index = block->var_count[V_UPVAL];

		block->var_count[V_UPVAL] += 1;

		block_push(block, B_UPVAL, (rt_value)temp, (rt_value)var, 0);

		kv_push(struct variable *, block->upvals, temp);

		return temp;
	}
}

static void generate_call(struct block *block, struct node *self, rt_value name, struct node *arguments, struct node *block_node, struct variable *var)
{
	struct variable *self_var = block_get_var(block);

	gen_node(block, self, self_var);

	int parameters = 0;

	struct variable *args = block_gen_args(block);

	struct variable *temp = var ? var : block_get_var(block);

	if(arguments)
		parameters = gen_argument(block, arguments, temp);

	struct variable* block_var = 0;

	if(block_node)
	{
		block_var = block_get_var(block);
		struct block *block_attach = gen_block(block_node);

		khiter_t k;
		khash_t(block) *hash = block_attach->variables;
		size_t upvals = 0;
		struct variable *closure_args = block_gen_args(block);

		for (k = kh_begin(hash); k != kh_end(hash); ++k)
		{
			if(kh_exist(hash, k))
			{
				struct variable *var = kh_value(hash, k);

				if(var->type == V_UPVAL)
				{
					khiter_t k = kh_get(block, block->variables, var->name);

					if(k != kh_end(block->variables) && kh_value(block->variables, k)->real == var->real)
						block_push(block, B_PUSH_UPVAL, (rt_value)kh_value(block->variables, k), 0, 0);
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

static void gen_call(struct block *block, struct node *node, struct variable *var)
{
	generate_call(block, node->left, (rt_value)node->middle, node->right->left, node->right->right, var);
}

static void gen_array_call(struct block *block, struct node *node, struct variable *var)
{
	generate_call(block, node->left, rt_symbol_from_cstr("[]"), node->middle, node->right, var);
}

static size_t gen_array_element(struct block *block, struct node *node, struct variable *var)
{
	gen_node(block, node->left, var);

	block_push(block, B_PUSH, (rt_value)var, 0, 0);

	if(node->right)
		return 1 + gen_array_element(block, node->right, var);
	else
		return 1;
}

static void gen_array(struct block *block, struct node *node, struct variable *var)
{
	struct variable *args = block_gen_args(block);

	size_t elements = node->left ? gen_array_element(block, node->left, var ? var : block_get_var(block)) : 0;

	block_end_args(block, args, elements);

	block_push(block, B_ARRAY, (rt_value)var, 0, 0);
}

static void gen_boolean(struct block *block, struct node *node, struct variable *var)
{
	struct variable *temp = var ? var : block_get_var(block);

	gen_node(block, node->left, temp);

	opcode_t *label_end = block_get_label(block);

	block_push(block, B_TEST, (rt_value)temp, 0, 0);
	block_push(block, (node->op == T_OR || node->op == T_OR_BOOLEAN) ? B_JMPT : B_JMPF, (rt_value)label_end, 0, 0);

	gen_node(block, node->right, var);

	block_emmit_label(block, label_end);
}

static void gen_not(struct block *block, struct node *node, struct variable *var)
{
	if(var)
	{
		struct variable *temp = var ? var : block_get_var(block);

		gen_node(block, node->left, temp);

		opcode_t *label_true = block_get_label(block);
		opcode_t *label_end = block_get_label(block);

		block_push(block, B_TEST, (rt_value)temp, 0, 0);

		block_push(block, B_JMPT, (rt_value)label_true, 0, 0);

		block_push(block, B_MOV_IMM, (rt_value)var, RT_TRUE, 0);
		block_push(block, B_JMP, (rt_value)label_end, 0, 0);

		block_emmit_label(block, label_true);
		block_push(block, B_MOV_IMM, (rt_value)var, RT_FALSE, 0);

		block_emmit_label(block, label_end);
	}
	else
		gen_node(block, node->left, 0);
}

static void gen_warn(struct block *block, struct node *node, struct variable *var)
{
	printf("node %d entered in code generation\n", node->type);
}

static void gen_ivar(struct block *block, struct node *node, struct variable *var)
{
	if(var)
	{
		block->self_ref++;
		block_push(block, B_GET_IVAR, (rt_value)var, (rt_value)node->left, 0);
	}
}

static void gen_ivar_assign(struct block *block, struct node *node, struct variable *var)
{
	struct variable *temp = var ? var : block_get_var(block);

	gen_node(block, node->right, temp);

	block->self_ref++;

	block_push(block, B_SET_IVAR, (rt_value)node->left, (rt_value)temp, 0);
}

static void gen_no_equality(struct block *block, struct node *node, struct variable *var)
{
	if(var)
	{
		struct variable *temp1 = var;
		struct variable *temp2 = block_get_var(block);

		gen_node(block, node->left, temp1);
		gen_node(block, node->right, temp2);

		opcode_t *label_true = block_get_label(block);
		opcode_t *label_end = block_get_label(block);

		block_push(block, B_CMP, (rt_value)temp1, (rt_value)temp2, 0);

		block_push(block, B_JMPE, (rt_value)label_true, 0, 0);

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

static bool has_ensure_block(struct block *block)
{
	struct exception_block *exception_block = block->current_exception_block;

	while(exception_block)
	{
		if(exception_block->ensure_label != (void *)-1)
			return true;

		exception_block = exception_block->parent;
	}

	return false;
}

static void gen_return(struct block *block, struct node *node, struct variable *var)
{
	struct variable *temp = var ? var : block_get_var(block);

	gen_node(block, node->left, temp);

	if(block->type == S_CLOSURE)
	{
		//rt_value label = block_get_label(block);

		//block_push(block, B_TEST_PROC, 0, 0, 0);
		//block_push(block, B_JMPNE, label, 0, 0);
		block_push(block, B_RAISE_RETURN, (rt_value)temp, (rt_value)block->owner->data, 0);

		//block_emmit_label(block, label);
	}
	else if(has_ensure_block(block))
		block_push(block, B_RAISE_RETURN, (rt_value)temp, (rt_value)block->data, 0);
	else
		block_push(block, B_RETURN, (rt_value)temp, 0, 0);
}

static void gen_break(struct block *block, struct node *node, struct variable *var)
{
	struct variable *temp = var ? var : block_get_var(block);

	gen_node(block, node->left, temp);

	block_push(block, B_RAISE_BREAK, (rt_value)temp, (rt_value)block->parent->data, block->break_id);
}

static void gen_next(struct block *block, struct node *node, struct variable *var)
{
	struct variable *temp = var ? var : block_get_var(block);

	gen_node(block, node->left, temp);

	block_push(block, B_RETURN, (rt_value)temp, 0, 0);
}

static void gen_redo(struct block *block, struct node *node, struct variable *var)
{
	block_push(block, B_REDO, 0, 0, 0);
}

static void gen_break_handler(struct block *block, struct node *node, struct variable *var)
{
	struct block *child = (void *)node->right;

	gen_node(block, node->left, var);

	opcode_t *label = 0;

	if(var)
	{
		/*
		 * Skip the break handler
		 */
		label = block_get_label(block);
		block_push(block, B_JMP, (rt_value)label, 0, 0);
	}

	/*
	 * Output target label
	 */
	block->data->break_targets[child->break_id] = (void *)block_emmit_label(block, block_get_flush_label(block));

	if(var)
	{
		block_push(block, B_STORE, (rt_value)var, 0, 0);
		block_emmit_label(block, label);
	}
}

static void gen_handler(struct block *block, struct node *node, struct variable *var)
{
	block->require_exceptions = true; // duh

	/*
	 * Allocate and setup the new exception block
	 */
	struct exception_block *exception_block = malloc(sizeof(struct exception_block));
	size_t index = kv_size(block->exception_blocks);
	size_t old_index = block->current_exception_block_id;

	exception_block->parent_index = old_index;
	exception_block->parent = block->current_exception_block;

	/*
	 * Check for ensure node
	 */
	if(node->right)
		exception_block->ensure_label = block_get_flush_label(block);
	else
		exception_block->ensure_label = 0;

	kv_init(exception_block->handlers);

	kv_push(struct exception_block *, block->exception_blocks, exception_block);

	/*
	 * Use the new exception block
	 */
	block->current_exception_block = exception_block;
	block->current_exception_block_id = index;
	block_push(block, B_HANDLER, index, 0, 0);

	/*
	 * Output the regular code
	 */
	exception_block->block_label = block_emmit_label(block, block_get_flush_label(block));

	gen_node(block, node->left, var);

	if(node->middle)
	{
		/*
		 * Skip the rescue block
		 */
		opcode_t *ok_label = block_get_label(block);
		block_push(block, B_JMP, (rt_value)ok_label, 0, 0);

		/*
		 * Output rescue node
		 */
		struct runtime_exception_handler *handler = malloc(sizeof(struct runtime_exception_handler));
		handler->common.type = E_RUNTIME_EXCEPTION;
		handler->rescue_label = block_emmit_label(block, block_get_flush_label(block));
		kv_push(struct exception_handler *, exception_block->handlers, (struct exception_handler *)handler);

		gen_node(block, node->middle->left, var);

		block_emmit_label(block, ok_label);
	}

	/*
	 * Restore the old exception frame
	 */
	block->current_exception_block_id = old_index;
	block->current_exception_block = exception_block->parent;
	block_push(block, B_HANDLER, old_index, 0, 0);

	/*
	 * Check for ensure node
	 */
	if(node->right)
	{
		exception_block->ensure_label = block_emmit_label(block, exception_block->ensure_label);

		/*
		 * Output ensure node
		 */
		gen_node(block, node->right, 0);

		block_push(block, B_ENSURE_RET, index, 0, 0);
	}
}

generator generators[] = {
	gen_unary_op,
	gen_binary_op,
	gen_num,
	gen_var,
	gen_ivar,
	gen_ivar_assign,
	gen_string,
	gen_string_start,
	gen_string_continue,
	gen_array,
	/*N_ARRAY_ELEMENT*/gen_warn,
	gen_const,
	gen_self,
	gen_true,
	gen_false,
	gen_nil,
	gen_assign,
	gen_const_assign,
	gen_boolean,
	gen_not,
	gen_no_equality,
	gen_if,
	gen_if,
	gen_return,
	gen_next,
	gen_redo,
	gen_break,
	gen_break_handler,
	gen_handler,
	/*N_RESCUE*/gen_warn,
	/*N_ARGUMENT*/gen_warn,
	/*N_CALL_ARGUMENTS*/gen_warn,
	gen_call,
	gen_array_call,
	gen_expressions,
	gen_class,
	gen_module,
	/*N_SCOPE*/gen_warn,
	gen_method
};

static inline void gen_node(struct block *block, struct node *node, struct variable *var)
{
	RT_ASSERT(node != 0);

	generators[node->type](block, node, var);
}

struct block *gen_block(struct node *node)
{
	struct block *block = (void *)node->left;

	struct variable *result = block_get_var(block);

	block->epilog = block_get_label(block);

	if(block->break_targets)
	{
		block->require_exceptions = true;
		block->data->break_targets = malloc(block->break_targets * sizeof(void *));
	}
	else
		block->data->break_targets = 0;

	gen_node(block, node->right, result);

	for(int i = 0; i < kv_size(block->upvals); i++)
		block_push(block, B_SEAL, (rt_value)kv_A(block->upvals, i), 0, 0);

	opcode_t *last = kv_A(block->vector, kv_size(block->vector) - 1);

	if(last->type == B_RETURN && last->left == (rt_value)result)
		last->type = B_LOAD;
	else
		block_push(block, B_LOAD, (rt_value)result, 0, 0);

	block_emmit_label(block, block->epilog);

	#ifdef DEBUG
		printf(";\n; block %x\n;\n", (rt_value)block);

		block_print(block);

		printf("\n\n");
	#endif

	return block;
}
