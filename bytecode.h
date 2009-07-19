#pragma once

#include "globals.h"

typedef enum {
	B_NOP,
	B_ADD,
	B_SUB,
	B_MUL,
	B_DIV,
	B_MOV,
	B_MOV_NUM,
	B_PUSH,
	B_PUSH_NUM,
	B_PUSH_OBJECT,
	B_CALL,
	B_TEST,
	B_JMPT,
	B_JMPF,
	B_JMP,
	B_RETURN,
	B_LABEL
} opcode_type_t;

typedef unsigned int OP_VAR;

typedef struct {
	opcode_type_t type;
	OP_VAR result;
	OP_VAR left;
	OP_VAR right;
} opcode_t;

KHASH_MAP_INIT_INT(OP_VAR, unsigned int);

typedef struct {
	OP_VAR var_count;
	OP_VAR label_count;
	kvec_t(opcode_t *) vector;
	khash_t(OP_VAR) *var_usage;
} block_t;

static inline block_t *block_create(void)
{
	block_t *result = malloc(sizeof(block_t));
	result->var_count = 3;
	kv_init(result->vector);
	result->var_usage = kh_init(OP_VAR);

	return result;
}

static inline void block_destroy(block_t *block)
{
	kh_destroy(OP_VAR, (block->var_usage));
	kv_destroy(block->vector);
	free(block);
}

static inline OP_VAR block_get_var(block_t *block)
{
	OP_VAR result = block->var_count;

	block->var_count += 2;

	return result;
}

static inline void block_use_var(block_t *block, OP_VAR var, unsigned int offset)
{
	int ret;

	khiter_t k = kh_put(OP_VAR, block->var_usage, var, &ret);

	if(!ret)
	{
		kh_del(OP_VAR, block->var_usage, k);

		printf("Unable to store use of variable %s\n", var);
	}
	else
		kh_value(block->var_usage, k) = offset;
}

static inline unsigned int block_push(block_t *block, opcode_type_t type, OP_VAR result, OP_VAR left, OP_VAR right)
{
	opcode_t *op = malloc(sizeof(opcode_t));
	op->type = type;
	op->result = result;
	op->left = left;
	op->right = right;
	kv_push(opcode_t *, block->vector, op);

	return kv_size(block->vector) - 1;
}

static inline void block_print_var(OP_VAR var)
{
	if(var & 1)
		printf("<%d>", (var - 1) / 2);
	else
		printf("%s", var);
}

static inline void block_print_label(OP_VAR var)
{
	printf("#%d", (var - 1) / 2);
}

void block_print(block_t *block);
void block_optimize(block_t *block);


