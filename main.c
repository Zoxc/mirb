#include "globals.h"
#include "compiler/parser/lexer.h"
#include "compiler/parser/parser.h"
#include "runtime/classes.h"
#include "runtime/classes/symbol.h"
#include "runtime/classes/string.h"
#include "compiler/generator/generator.h"
#include "compiler/generator/x86.h"

int main()
{
	char buffer[800];

	parser_setup();

	rt_create();

	while(1)
	{
		gets(buffer);

		struct parser *parser = parser_create(buffer, "Input");

		if(parser_current(parser) == T_EOF)
		{
			parser_destroy(parser);

			break;
		}

		node_t* expression = parse_main(parser);

		if(parser->err_count == 0)
		{
			#ifdef DEBUG
				printf("Parsing done.\n");
				printf("Tree: %s\n", rt_string_to_cstr(get_node_name(expression)));
			#endif

			block_t *block = gen_block(expression);

			rt_compiled_block_t compiled_block = compile_block(block);

			#ifdef DEBUG
				printf("Running block %x: ", (rt_value)compiled_block);
			#endif

			//__asm__("int3\n"); // Make debugging life easier

			rt_value result = compiled_block(rt_main, RT_NIL, 0, 0);

			printf("=> "); rt_print(result); printf("\n");
		}

		parser_destroy(parser);
	}

	printf("Exiting gracefully...");

	rt_destroy();

	return 0;
}
