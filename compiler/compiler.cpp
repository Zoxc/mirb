#include "compiler.hpp"

struct compiler *compiler_create()
{
	struct compiler *result = (struct compiler *)malloc(sizeof(struct compiler));

	allocator_init(&result->allocator);

	return result;
}

void compiler_destroy(struct compiler *compiler)
{
	allocator_free(&compiler->allocator);
	free(compiler);
}
