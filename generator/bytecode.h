#pragma once

#include "../globals.h"
#include "../runtime/classes.h"
#include "../runtime/classes/symbol.h"
#include "../parser/parser.h"

typedef enum {
	B_NOP,
	B_ADD,
	B_SUB,
	B_MUL,
	B_DIV,
	B_MOV,
	B_MOV_IMM,
	B_SELF,
	B_PUSH,
	B_PUSH_IMM,
	B_PUSH_RAW,
	B_CALL,
	B_ARGS,
	B_LOAD,
	B_STORE,
	B_TEST,
	B_JMPT,
	B_JMPF,
	B_JMP,
	B_RETURN,
	B_LABEL,
	B_ARRAY,
	B_STRING,
	B_INTERPOLATE,
	B_UPVAL,
	B_PUSH_UPVAL,
	B_SEAL,
	B_CLOSURE,
	B_GET_UPVAL,
	B_SET_UPVAL,
	B_GET_IVAR,
	B_SET_IVAR,
	B_GET_CONST,
	B_SET_CONST,
	B_CLASS,
	B_MODULE,
	B_METHOD
} opcode_type_t;

typedef struct {
	opcode_type_t type;
	rt_value result;
	rt_value left;
	rt_value right;
} opcode_t;

typedef struct {
	struct parser* parser;
	rt_value label_count;
	kvec_t(opcode_t *) vector;
	kvec_t(variable_t *) upvals;
	khash_t(rt_hash) *label_usage;
	scope_t *scope;
	size_t self_ref;
} block_t;

static inline block_t *block_create(scope_t *scope)
{
	block_t *result = parser_alloc(scope->parser, sizeof(block_t));

	result->parser = scope->parser;
	result->label_count = 0;
	result->scope = scope;
	result->label_usage = kh_init(rt_hash);
	result->self_ref = 0;

	kv_init(result->vector);
	kv_init(result->upvals);

	return result;
}

static inline void block_destroy(block_t *block)
{
	kh_destroy(rt_hash, block->label_usage);
	kv_destroy(block->vector);
	kv_destroy(block->upvals);
	free(block);
}

static inline rt_value block_get_label(block_t *block)
{
	return block->label_count++;
}

static inline variable_t *block_get_var(block_t *block)
{
	variable_t *temp = parser_alloc(block->parser, sizeof(variable_t));

	temp->type = V_TEMP;
	temp->index = block->scope->var_count[V_TEMP];

	block->scope->var_count[V_TEMP] += 1;

	return temp;
}

static inline size_t block_push(block_t *block, opcode_type_t type, rt_value result, rt_value left, rt_value right)
{
	opcode_t *op = parser_alloc(block->parser, sizeof(opcode_t));
	op->type = type;
	op->result = result;
	op->left = left;
	op->right = right;
	kv_push(opcode_t *, block->vector, op);

	return kv_size(block->vector) - 1;
}

static inline variable_t *block_gen_args(block_t *block)
{
	variable_t *var = parser_alloc(block->parser, sizeof(variable_t));

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

static inline void block_emmit_label(block_t *block, rt_value label)
{
	int index = block_push(block, B_LABEL, label, 0, 0);

	int ret;

	khiter_t k = kh_put(rt_hash, block->label_usage, label, &ret);

	assert(ret);

	kh_value(block->label_usage, k) = index;
}

static inline rt_value block_get_value(block_t *block, khash_t(rt_hash) *table, rt_value key)
{
	khiter_t k = kh_get(rt_hash, table, key);

	assert(k != kh_end(table));

	return kh_value(table, k);
}

static inline void block_print_label(rt_value label)
{
	printf("#%d", label);
}

const char *variable_name(rt_value var);

void block_print(block_t *block);

