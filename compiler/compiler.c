#include "compiler.h"

void compiler_setup(void)
{
	lexer_setup();
}

struct compiler *compiler_create(const char* input, const char *filename)
{
	struct compiler* result = malloc(sizeof(struct compiler));

	result->filename = filename;
	result->err_count = 0;
	result->current_block = 0;

    allocator_init(&result->allocator);

    lexer_create(result, input);

    return result;
}

void compiler_destroy(struct compiler *compiler)
{
	allocator_free(&compiler->allocator);
	free(compiler);
}
