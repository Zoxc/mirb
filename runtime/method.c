#include "symbol.h"
#include "method.h"
#include "string.h"
#include "../globals.h"

void __stdcall rt_method_create_main(rt_value name, rt_compiled_block_t block)
{
	rt_method_create(rt_Object, name, block);
}

void __stdcall rt_method_create(rt_value under, rt_value name, rt_compiled_block_t block)
{
	rt_value under_path = rt_object_get_var(under, rt_symbol_from_cstr("__classpath__"));

	printf("Defining method %s.%s\n", rt_string_to_cstr(under_path), rt_symbol_to_cstr(name));

	rt_class_set_method(under, name, block);
}
