#include "generator.hpp"
#include "../../runtime/classes.hpp"
#include "../../runtime/classes/symbol.hpp"
#include "../../runtime/classes/fixnum.hpp"
#include "../../runtime/support.hpp"

typedef void(*generator)(struct block *block, struct node *node, struct variable *var);

static inline void gen_node(struct block *block, struct node *node, struct variable *var);


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

static struct variable *generate_block_arg(struct block *block, struct node *block_node)
{
	struct variable* closure = 0;

	if(block_node)
	{
		block->self_ref++;

		closure = block_get_var(block);
		struct block *block_attach = gen_block(block_node);

		for(size_t i = 0; i < block_attach->scopes.size; i++)
			block_push(block, B_PUSH_SCOPE, (rt_value)block_attach->scopes.array[i], 0, 0);

		struct opcode *args = block_push(block, B_ARGS, block_attach->scopes.size, 0, 0);

		block_push(block, B_CLOSURE, (rt_value)closure, (rt_value)block_attach, 0);

		block_push(block, B_ARGS_POP, (rt_value)args, 5, 0);
	}

	return closure;
}

static void generate_block_arg_seal(struct block *block, struct variable *closure)
{
	if(closure)
		block_push(block, B_SEAL, (rt_value)closure, 0, 0);
}

struct call_args_info
{
	struct variable *closure;
	struct opcode *args;
};

static struct call_args_info generate_call_args(struct block *block, struct node *arguments, struct node *block_node, struct variable *var)
{
	struct call_args_info result;

	int parameters = 0;

	struct variable *temp = var ? var : block_get_var(block);

	if(arguments)
		parameters = gen_argument(block, arguments, temp);

	result.closure = generate_block_arg(block, block_node);
	result.args = block_push(block, B_CALL_ARGS, parameters, (rt_value)result.closure, 0);

	return result;
}

static void generate_call_args_seal(struct block *block, struct call_args_info *info)
{
	block_push(block, B_CALL_ARGS_POP, (rt_value)info->args, 0, 0);

	generate_block_arg_seal(block, info->closure);
}

static struct call_args_info generate_unary_call_args(struct block *block, struct variable *var)
{
	return generate_call_args(block, 0, 0, var);
}

static struct call_args_info generate_binary_call_args(struct block *block, struct node *arg, struct variable *var)
{
	struct node argument;

	argument.left = arg;
	argument.right = 0;

	return generate_call_args(block, &argument, 0, var);
}

static void gen_unary_op(struct block *block, struct node *node, struct variable *var)
{
	struct variable *temp = var ? var : block_get_var(block);

	gen_node(block, node->left, temp);

	struct call_args_info info = generate_unary_call_args(block, temp);

	block_push(block, B_CALL, (rt_value)temp, rt_symbol_from_cstr(token_type_names[node->op]), 0);

	generate_call_args_seal(block, &info);

	if (var)
		block_push(block, B_STORE, (rt_value)var, 0, 0);
}

static void gen_binary_op(struct block *block, struct node *node, struct variable *var)
{
	struct variable *temp = var ? var : block_get_var(block);

	gen_node(block, node->left, temp);

	struct call_args_info info = generate_binary_call_args(block, node->right, 0);

	block_push(block, B_CALL, (rt_value)temp, rt_symbol_from_cstr(token_type_names[node->op]), 0);

	generate_call_args_seal(block, &info);

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
		block_push(block, B_MOV, (rt_value)var, (rt_value)node->left, 0);
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

	if(strlen((const char *)node->middle))
	{
		block_push(block, B_STRING, (rt_value)temp, (rt_value)node->middle, 0);
		block_push(block, B_PUSH, (rt_value)temp, 0, 0);
	}

	struct variable *interpolated = block_get_var(block);

	gen_node(block, node->right, interpolated);

	block_push(block, B_PUSH, (rt_value)interpolated, 0, 0);
}

static size_t gen_string_arg_count(struct node *node)
{
	return 1 + (strlen((const char *)node->middle) > 0) + (node->left ? gen_string_arg_count(node->left) : 0);
}

static void gen_string_start(struct block *block, struct node *node, struct variable *var)
{
	struct variable *temp = var ? var : block_get_var(block);

	gen_node(block, node->left, temp);

	size_t present = strlen((const char *)node->right) > 0;

	if(present)
	{
		block_push(block, B_STRING, (rt_value)temp, (rt_value)node->right, 0);
		block_push(block, B_PUSH, (rt_value)temp, 0, 0);
	}

	struct opcode *args = block_push(block, B_ARGS, gen_string_arg_count(node->left) + present, 0, 0);

	block_push(block, B_INTERPOLATE, (rt_value)var, 0, 0);

	block_push(block, B_ARGS_POP, (rt_value)args, 2, 0);
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
	gen_node(block, node->right, (struct variable *)node->left);

	if (var)
		block_push(block, B_MOV, (rt_value)var, (rt_value)node->left, 0);
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
	struct opcode *label_else = block_get_label(block);

	gen_node(block, node->left, temp);

	block_push(block, B_TEST, (rt_value)temp, 0, 0);
	block_push(block, node->type == N_IF ? B_JMPF : B_JMPT, (rt_value)label_else, 0, 0);

	struct opcode *label_end = block_get_label(block);

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

	if(node->middle)
	{
		struct variable *temp = var ? var : block_get_var(block);

		gen_node(block, node->middle, temp);

		block_push(block, B_CLASS, (rt_value)node->left, (rt_value)temp, (rt_value)gen_block(node->right));
	}
	else
		block_push(block, B_CLASS, (rt_value)node->left, 0, (rt_value)gen_block(node->right));

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

static void generate_call(struct block *block, struct node *self, rt_value name, struct node *arguments, struct node *block_node, struct variable *var)
{
	struct variable *self_var = block_get_var(block);

	gen_node(block, self, self_var);

	struct call_args_info info = generate_call_args(block, arguments, block_node, var);

	block_push(block, B_CALL, (rt_value)self_var, name, (rt_value)info.closure);

	generate_call_args_seal(block, &info);

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
	size_t elements = node->left ? gen_array_element(block, node->left, var ? var : block_get_var(block)) : 0;

	struct opcode *args = block_push(block, B_ARGS, (rt_value)elements, 0, 0);

	block_push(block, B_ARRAY, (rt_value)var, 0, 0);

	block_push(block, B_ARGS_POP, (rt_value)args, 2, 0);
}

static void gen_boolean(struct block *block, struct node *node, struct variable *var)
{
	struct variable *temp = var ? var : block_get_var(block);

	gen_node(block, node->left, temp);

	struct opcode *label_end = block_get_label(block);

	block_push(block, B_TEST, (rt_value)temp, 0, 0);
	block_push(block, (node->op == T_OR || node->op == T_LOGICAL_OR) ? B_JMPT : B_JMPF, (rt_value)label_end, 0, 0);

	gen_node(block, node->right, var);

	block_emmit_label(block, label_end);
}

static void gen_not(struct block *block, struct node *node, struct variable *var)
{
	if(var)
	{
		struct variable *temp = var ? var : block_get_var(block);

		gen_node(block, node->left, temp);

		struct opcode *label_true = block_get_label(block);
		struct opcode *label_end = block_get_label(block);

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

		struct opcode *label_true = block_get_label(block);
		struct opcode *label_end = block_get_label(block);

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
	struct block *child = (struct block *)node->right;

	gen_node(block, node->left, var);

	struct opcode *label = 0;

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
	struct exception_block *exception_block = (struct exception_block *)malloc(sizeof(struct exception_block));
	size_t index = block->exception_blocks.size;
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

	vec_init(rt_exception_handlers, &exception_block->handlers);

	vec_push(rt_exception_blocks, &block->exception_blocks, exception_block);

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
		struct opcode *ok_label = block_get_label(block);
		block_push(block, B_JMP, (rt_value)ok_label, 0, 0);

		/*
		 * Output rescue node
		 */
		struct runtime_exception_handler *handler = (struct runtime_exception_handler *)malloc(sizeof(struct runtime_exception_handler));
		handler->common.type = E_RUNTIME_EXCEPTION;
		handler->rescue_label = block_emmit_label(block, block_get_flush_label(block));
		vec_push(rt_exception_handlers, &exception_block->handlers, (struct exception_handler *)handler);

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
		exception_block->ensure_label = block_emmit_label(block, (struct opcode *)exception_block->ensure_label);

		/*
		 * Output ensure node
		 */
		gen_node(block, node->right, 0);

		block_push(block, B_ENSURE_RET, index, 0, 0);
	}
}

static void gen_super(struct block *block, struct node *node, struct variable *var)
{
    block->self_ref++;

	struct call_args_info info = generate_call_args(block, node->left, node->right, var);

	block_push(block, B_SUPER, (rt_value)info.closure, 0, 0);

	generate_call_args_seal(block, &info);

	if (var)
		block_push(block, B_STORE, (rt_value)var, 0, 0);
}

static void gen_zsuper(struct block *block, struct node *node, struct variable *var)
{
    block->self_ref++;

	struct variable* closure = node->right ? (struct variable *)node->right : block->owner->block_parameter;

	/*
	 * Push arguments
	 */

	for(size_t i = 0; i < block->owner->parameters.size; i++)
	{
		block_push(block, B_PUSH, (rt_value)block->owner->parameters.array[i], 0, 0);
	}

	struct opcode *args = block_push(block, B_CALL_ARGS, (rt_value)block->owner->parameters.size, 0, 0);

	block_push(block, B_SUPER, (rt_value)closure, 0, 0);

	block_push(block, B_CALL_ARGS_POP, (rt_value)args, 0, 0);

	generate_block_arg_seal(block, closure);

	if (var)
		block_push(block, B_STORE, (rt_value)var, 0, 0);
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
	gen_super,
	gen_zsuper,
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
	struct block *block = (struct block *)node->left;

	struct variable *result = block_get_var(block);

	for(size_t i = 0; i < block->zsupers.size; i++)
	{
		block_require_args(block->zsupers.array[i], block);
	}

	block->epilog = block_get_label(block);

	if(block->break_targets)
	{
		block->require_exceptions = true;
		block->data->break_targets = (void **)malloc(block->break_targets * sizeof(void *));
	}
	else
		block->data->break_targets = 0;

	gen_node(block, node->right, result);

	struct opcode *last = block->opcodes.array[block->opcodes.size - 1];

	if(last->type == B_RETURN && last->left == (rt_value)result)
		last->type = B_LOAD;
	else
		block_push(block, B_LOAD, (rt_value)result, 0, 0);

	block_emmit_label(block, block->epilog);

	/*
	 * Assign an unique id to each variable of type V_HEAP and V_LOCAL
	 */

	for(hash_iter_t i = hash_begin(block->variables); i != hash_end(block->variables); i++)
	{
		if(hash_exist(block->variables, i))
		{
			struct variable *var = hash_value(block->variables, i);

			switch(var->type)
			{
				case V_HEAP:
				case V_LOCAL:
					var->index = block->var_count[var->type]++;
					break;

				default:
					break;
			}
		}
	}

	#ifdef DEBUG
		printf(";\n; block %x\n;\n", (rt_value)block);

		block_print(block);

		printf("\n\n");
	#endif

	return block;
}
