#include "runtime.h"
#include "symbol.h"
#include "string.h"
#include "code_heap.h"
#include "bool.h"

void rt_create(void)
{
	rt_code_heap_create();
	rt_symbols_create();

	rt_setup_classes();
	rt_setup_bool();

	rt_symbol_init();

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

	printf("Undefined method %s on %s\n", rt_symbol_to_cstr(name), rt_string_to_cstr(rt_to_s(obj)));
	assert(0);

	return 0;
}

rt_value rt_to_s(rt_value obj)
{
	rt_compiled_block_t to_s = rt_lookup(obj, rt_symbol_from_cstr("to_s"));

	if(to_s)
		return to_s(obj, 0);

	switch(rt_type(obj))
	{
		case C_FIXNUM:
			printf("%d", RT_FIX2INT(obj));
			break;

		case C_TRUE:
			return rt_string_from_cstr("true");

		case C_FALSE:
			return rt_string_from_cstr("false");

		case C_NIL:
			return rt_string_from_cstr("nil");

		case C_SYMBOL:
			return rt_symbol_to_s(obj, 0);

		case C_CLASS:
			return rt_class_to_s(obj, 0);

		case C_OBJECT:
			return rt_object_to_s(obj, 0);

		case C_STRING:
			return obj;

		default:
			break;
	}

	assert(0);
}

void rt_print(rt_value obj)
{
	rt_value string = rt_to_s(obj);

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
