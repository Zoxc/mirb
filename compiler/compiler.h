#pragma once
#include "../globals.h"
#include "kvec_cp.h"
#include "allocator.h"
#include "lexer.h"

struct block;

struct compiler {
	/*
	 * Parameters
	 */
	const char* filename;

	/*
	 * Parser stuff
	 */
	struct token token;
	int index;
	int count;
	int err_count;
	struct block *current_block;

	/*
	 * Memory allocator
	 */
	struct allocator allocator;
};

#define COMPILER_ERROR(compiler, msg, ...) do \
	{ \
		compiler->err_count++; \
		printf("(%s: %d) "msg"\n", compiler->filename ? compiler->filename : "Line ", lexer_token(compiler)->line + 1, ##__VA_ARGS__); \
	} \
	while(0)

void compiler_setup(void);
struct compiler *compiler_create(const char* input, const char *filename);
void compiler_destroy(struct compiler *compiler);

static inline void *compiler_alloc(struct compiler *compiler, size_t length)
{
	return allocator_alloc(&compiler->allocator, length);
}
