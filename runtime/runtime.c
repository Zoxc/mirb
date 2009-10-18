#include "runtime.h"
#include "code_heap.h"
#include "classes/symbol.h"
#include "classes/string.h"
#include "classes/bool.h"
#include "classes/fixnum.h"
#include "classes/proc.h"
#include "classes/module.h"
#include "classes/array.h"
#include "classes/exception.h"
#include "modules/kernel.h"
#include "../generator/bytecode.h"
#include "../generator/generator.h"
#include "../generator/x86.h"

void rt_create(void)
{
	rt_code_heap_create();
	rt_symbols_create();

	rt_setup_classes();

	rt_class_init();
	rt_object_init();
	rt_module_init();

	rt_kernel_init();

	rt_bool_init();
	rt_symbol_init();
	rt_string_init();
	rt_fixnum_init();
	rt_proc_init();
	rt_array_init();
	rt_exception_init();
}

void rt_destroy(void)
{
	rt_symbols_destroy();
	rt_code_heap_destroy();

	kh_destroy(rt_hash, object_var_hashes);
}

rt_value rt_eval(rt_value self, const char *input, const char *filename)
{
	struct parser *parser = parser_create(input, filename);

	struct node* expression = parse_main(parser);

	if(parser->err_count != 0)
		return RT_NIL;

	extern rt_value get_node_name(struct node *node);

	#ifdef DEBUG
		printf("Tree: %s\n", rt_string_to_cstr(get_node_name(expression)));
	#endif

	block_t *code_block = gen_block(expression);

	rt_compiled_block_t compiled_block = compile_block(code_block);

	parser_destroy(parser);

	return compiled_block(self, RT_NIL, 0, 0);
}

rt_compiled_block_t rt_lookup_nothrow(rt_value obj, rt_value name)
{
	rt_value c = rt_class_of(obj);

	do
	{
		rt_compiled_block_t result = rt_class_get_method(c, name);

		if(result)
			return result;

		c = RT_CLASS(c)->super;
	}
	while(c != 0);

	return 0;
}

rt_compiled_block_t rt_lookup(rt_value obj, rt_value name)
{
	rt_compiled_block_t result = rt_lookup_nothrow(obj, name);

	if(!result)
	{
		printf("Undefined method %s on %s\n", rt_symbol_to_cstr(name), rt_string_to_cstr(rt_inspect(obj)));
		assert(0);
	}

	return result;
}

rt_value rt_inspect(rt_value obj)
{
	rt_compiled_block_t inspect = rt_lookup_nothrow(obj, rt_symbol_from_cstr("inspect"));

	rt_value result = RT_NIL;

	if(inspect && (inspect != (rt_compiled_block_t)rt_object_inspect || rt_lookup_nothrow(obj, rt_symbol_from_cstr("to_s"))))
		result = inspect(obj, 0, 0, 0);

	if(rt_type(result) == C_STRING)
		return result;
	else
		return rt_object_to_s(obj, 0, 0, 0);
}

void rt_print(rt_value obj)
{
	rt_value string = rt_inspect(obj);

	printf("%s", rt_string_to_cstr(string));
}

rt_value rt_dump_call(rt_value obj, size_t argc, ...)
{
	printf("Dumped call to "); rt_print(obj); printf(".(");

	va_list args;

	argc--;

	va_start(args, argc);

	for(int i = 0; i < argc; i++)
	{
		rt_value arg = va_arg(args, rt_value);

		rt_print(arg);

		if(i != argc - 1)
			printf(", ");
	}

	va_end(args);

	printf(") -> nil\n");

	return RT_NIL;
}
