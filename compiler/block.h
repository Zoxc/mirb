#pragma once
#include "ast.h"
#include "bytecode.h"
#include "compiler.h"
#include "../runtime/runtime.h"

/*
 * Exception runtime data
 */

typedef enum {
	E_RUNTIME_EXCEPTION,
	E_CLASS_EXCEPTION,
	E_FILTER_EXCEPTION,
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

typedef struct block_data {
	kvec_t(exception_block_t *) exception_blocks;
	size_t local_storage;
	void *epilog;
} block_data_t;

/*
 * Variables
 */

typedef enum {
	V_PARAMETER,
	V_LOCAL,
	V_UPVAL,
	V_TEMP,
	V_BLOCK,
	V_ARGS
} variable_type;

#define VARIABLE_TYPES 6

typedef struct variable_t {
	struct variable_t *real;
	rt_value index;
	rt_value name;
	variable_type type;
} variable_t;

KHASH_MAP_INIT_INT(block, variable_t *);

/*
 * Block definition
 */

enum block_type {
	S_MAIN,
	S_METHOD,
	S_CLASS,
	S_MODULE,
	S_CLOSURE
};

typedef struct block {
	enum block_type type;
	struct compiler *compiler; // The compiler which owns this block

	bool can_break; // If this block can raise a exception because of a break.

	struct block_data *data; // Runtime data

	/*
	 * parser stuff
	 */
	rt_value var_count[VARIABLE_TYPES];
	khash_t(block) *variables;
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
	size_t self_ref;
	bool require_exceptions;
	size_t epilog; // The end of the block
} block_t;

/*
 * Block functions
 */

block_t *block_create(struct compiler *compiler, enum block_type type);

static inline rt_value block_get_label(block_t *block)
{
	return block->label_count++;
}

static inline variable_t *block_get_var(block_t *block)
{
	variable_t *temp = compiler_alloc(block->compiler, sizeof(variable_t));

	temp->type = V_TEMP;
	temp->index = block->var_count[V_TEMP];

	block->var_count[V_TEMP] += 1;

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
	var->index = block->var_count[V_ARGS];

	block->var_count[V_ARGS] += 1;

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

