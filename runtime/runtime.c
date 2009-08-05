#include "runtime.h"
#include "symbol.h"
#include "string.h"
#include "code_heap.h"
#include "bool.h"
#include "fixnum.h"

void rt_create(void)
{
	rt_code_heap_create();
	rt_symbols_create();

	rt_setup_classes();

	rt_bool_init();
	rt_symbol_init();
	rt_string_init();
	rt_fixnum_init();
}

void rt_destroy(void)
{
	rt_symbols_destroy();
	rt_code_heap_destroy();

	kh_destroy(rt_hash, object_var_hashes);
}

rt_compiled_block_t rt_lookup(rt_value obj, rt_value name)
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

rt_value rt_inspect(rt_value obj)
{
	rt_compiled_block_t inspect = rt_lookup(obj, rt_symbol_from_cstr("inspect"));

	if(inspect)
		return inspect(obj, 0);

	return rt_object_inspect(obj, 0);
}

void rt_print(rt_value obj)
{
	rt_value string = rt_inspect(obj);

	printf("%s", rt_string_to_cstr(string));
}

rt_value rt_dump_call(rt_value obj, unsigned int argc, ...)
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
