#include "compiler.h"

struct compiler *compiler_create()
{
	struct compiler* result = malloc(sizeof(struct compiler));

	allocator_init(&result->allocator);

	return result;
}

void compiler_destroy(struct compiler *compiler)
{
	allocator_free(&compiler->allocator);
	free(compiler);
}
