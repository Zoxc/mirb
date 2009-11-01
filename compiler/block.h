#pragma once
#include "ast.h"
#include "bytecode.h"
#include "compiler.h"
#include "parser/parser.h"
#include "../runtime/runtime.h"

/*
 * Exception runtime data
 */

typedef enum {
	E_RUNTIME_EXCEPTION,
	E_CLASS_EXCEPTION,
	E_FILTER_EXCEPTION,
	E_RETURN
} exception_handler_type_t;

typedef struct exception_handler {
	exception_handler_type_t type;
	struct exception_handler *next;
} exception_handler_t;

typedef struct {
	exception_handler_t common;
	void *rescue_label;
} runtime_exception_handler_t;

typedef struct {
	runtime_exception_handler_t common;
} class_exception_handler_t;

typedef struct exception_block {
	size_t parent_index;
	struct exception_block *parent;
	kvec_t(exception_handler_t *) handlers;
	void *block_label;
	void *ensure_label;
} exception_block_t;

/*
 * Block runtime data
 */

typedef struct {
	rt_compiled_block_t block;
	kvec_t(exception_block_t *) exception_blocks;
	size_t local_storage;
	void *epilog;
} block_data_t;

/*
 * Block definition
 */

typedef struct block {
	struct compiler *compiler; // The compiler which owns this block

	/*
	 * parser stuff
	 */
	scope_type type;
	rt_value var_count[VARIABLE_TYPES];
	khash_t(scope) *variables;
	struct block *owner;
	struct block *parent;
	variable_t *block_var;

	/*
	 * bytecode compiler stuff
	 */
	rt_value label_count;
	kvec_t(opcode_t *) vector;
	kvec_t(variable_t *) upvals;
	kvec_t(exception_block_t *) exception_blocks;
	exception_block_t *current_exception_block;
	size_t local_offset;
	size_t current_exception_block_id;
	khash_t(rt_hash) *label_usage;
	scope_t *scope;
	size_t self_ref;
	bool require_exceptions;
} block_t;

/*
 * Block functions
 */

static inline block_t *block_create(scope_t *scope)
{
	block_t *result = compiler_alloc(scope->compiler, sizeof(block_t));

	result->compiler = scope->compiler;
	result->label_count = 0;
	result->scope = scope;
	result->label_usage = kh_init(rt_hash);
	result->self_ref = 0;
	result->require_exceptions = false;
	result->current_exception_block = 0;
	result->current_exception_block_id = (size_t)-1;

	kv_init(result->vector);
	kv_init(result->exception_blocks);
	kv_init(result->upvals);

	return result;
}

static inline void block_destroy(block_t *block)
{
	kh_destroy(rt_hash, block->label_usage);
	kv_destroy(block->vector);
	kv_destroy(block->upvals);
	kv_destroy(block->exception_blocks);
	free(block);
}

static inline rt_value block_get_label(block_t *block)
{
	return block->label_count++;
}

static inline variable_t *block_get_var(block_t *block)
{
	variable_t *temp = compiler_alloc(block->compiler, sizeof(variable_t));

	temp->type = V_TEMP;
	temp->index = block->scope->var_count[V_TEMP];

	block->scope->var_count[V_TEMP] += 1;

	return temp;
}

static inline size_t block_push(block_t *block, opcode_type_t type, rt_value result, rt_value left, rt_value right)
{
	opcode_t *op = compiler_alloc(block->compiler, sizeof(opcode_t));
	op->type = type;
	op->result = result;
	op->left = left;
	op->right = right;
	kv_push(opcode_t *, block->vector, op);

	return kv_size(block->vector) - 1;
}

static inline variable_t *block_gen_args(block_t *block)
{
	variable_t *var = compiler_alloc(block->compiler, sizeof(variable_t));

	var->type = V_ARGS;
	var->index = block->scope->var_count[V_ARGS];

	block->scope->var_count[V_ARGS] += 1;

	block_push(block, B_ARGS, (rt_value)var, (rt_value)false, 0);

	return var;
}

static inline void block_end_args(block_t *block, variable_t *var, size_t argc)
{
	block_push(block, B_ARGS, (rt_value)var, (rt_value)true, argc);
}

static inline size_t block_emmit_label_type(block_t *block, rt_value label, label_type_t label_type)
{
	size_t index = block_push(block, B_LABEL, label, label_type, 0);

	int ret;

	khiter_t k = kh_put(rt_hash, block->label_usage, label, &ret);

	RT_ASSERT(ret);

	kh_value(block->label_usage, k) = index;

	return index;
}

static inline size_t block_emmit_label(block_t *block, rt_value label)
{
	size_t index = block_push(block, B_LABEL, label, 0, 0);

	int ret;

	khiter_t k = kh_put(rt_hash, block->label_usage, label, &ret);

	RT_ASSERT(ret);

	kh_value(block->label_usage, k) = index;

	return index;
}

static inline rt_value block_get_value(block_t *block, khash_t(rt_hash) *table, rt_value key)
{
	khiter_t k = kh_get(rt_hash, table, key);

	RT_ASSERT(k != kh_end(table));

	return kh_value(table, k);
}

static inline void block_print_label(rt_value label)
{
	printf("#%d", label);
}

const char *variable_name(rt_value var);

void block_print(block_t *block);

