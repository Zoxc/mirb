#pragma once

#include "../globals.h"
#include "../runtime/classes.h"
#include "../runtime/symbol.h"
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
	B_CALL,
	B_CALL_SPLAT,
	B_LOAD,
	B_STORE,
	B_TEST,
	B_TEST_IMM, //TODO: Remove this
	B_JMPT,
	B_JMPF,
	B_JMP,
	B_RETURN,
	B_LABEL,
	B_STRING,
	B_INTERPOLATE,
	B_GET_CONST,
	B_SET_CONST,
	B_CLASS,
	B_METHOD
} opcode_type_t;

typedef struct {
	opcode_type_t type;
	rt_value result;
	rt_value left;
	rt_value right;
} opcode_t;

typedef struct {
	rt_value next_temp;
	rt_value label_count;
	kvec_t(opcode_t *) vector;
	khash_t(rt_hash) *var_usage;
	khash_t(rt_hash) *label_usage;
	scope_t *scope;
	unsigned int self_ref;
} block_t;

static inline block_t *block_create(scope_t *scope)
{
	block_t *result = malloc(sizeof(block_t));

	result->next_temp = scope->next_local;
	result->label_count = 0;
	result->scope = scope;
	result->var_usage = kh_init(rt_hash);
	result->label_usage = kh_init(rt_hash);
	result->self_ref = 0;

	kv_init(result->vector);

	return result;
}

static inline void block_destroy(block_t *block)
{
	kh_destroy(rt_hash, block->var_usage);
	kh_destroy(rt_hash, block->label_usage);
	kv_destroy(block->vector);
	free(block);
}

static inline rt_value block_get_label(block_t *block)
{
	return block->label_count++;
}

static inline rt_value block_get_var(block_t *block)
{
	rt_value result = block->next_temp++;

	return result;
}

static inline void block_use_var(block_t *block, rt_value var, unsigned int offset)
{
	int ret;

	khiter_t k = kh_put(rt_hash, block->var_usage, var, &ret);

	assert(ret);

	kh_value(block->var_usage, k) = offset;
}

static inline unsigned int block_push(block_t *block, opcode_type_t type, rt_value result, rt_value left, rt_value right)
{
	opcode_t *op = malloc(sizeof(opcode_t));
	op->type = type;
	op->result = result;
	op->left = left;
	op->right = right;
	kv_push(opcode_t *, block->vector, op);

	return kv_size(block->vector) - 1;
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

static inline void block_print_var(rt_value var)
{
	printf("%%%d", var);
}

static inline void block_print_label(rt_value label)
{
	printf("#%d", label);
}

void block_print(block_t *block);

