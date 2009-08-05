#include "classes.h"
#include "runtime.h"
#include "symbol.h"
#include "string.h"
#include "constant.h"

rt_value __stdcall rt_support_get_const(rt_value obj, rt_value name)
{
	printf("Looking up constant %s in %s\n", rt_symbol_to_cstr(name), rt_string_to_cstr(rt_to_s(obj)));

	if(obj == rt_main)
		obj = rt_Object;

	return rt_const_get(obj, name);
}

void __stdcall rt_support_set_const(rt_value obj, rt_value name, rt_value value)
{
	printf("Setting constant %s in %s to %s\n", rt_symbol_to_cstr(name), rt_string_to_cstr(rt_to_s(obj)), rt_string_to_cstr(rt_to_s(value)));

	if(obj == rt_main)
		obj = rt_Object;

	rt_const_set(obj, name, value);
}

rt_value __stdcall rt_support_class_create(rt_value under, rt_value name, rt_value super)
{
	if(under == rt_main)
		under = rt_Object;

	return rt_define_class(under, name, super);
}

void __stdcall rt_support_method_create(rt_value obj, rt_value name, rt_compiled_block_t block)
{
	if(obj == rt_main)
		obj = rt_Object;

	rt_define_method(obj, name, block);
}

rt_compiled_block_t __cdecl rt_support_lookup_method(rt_value method, rt_value obj)
{
	return rt_lookup(obj, method);
}


