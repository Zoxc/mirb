#include "../classes.h"
#include "../runtime.h"
#include "../constant.h"
#include "symbol.h"
#include "string.h"

rt_value rt_Module;

rt_value __cdecl rt_module_to_s(rt_value obj, size_t argc)
{
	rt_value name = rt_object_get_var(obj, rt_symbol_from_cstr("__classname__"));

	if(rt_test(name))
		return name;

	return rt_object_to_s(obj, 0);
}

rt_value __cdecl rt_module_append_features(rt_value obj, size_t argc, rt_value target)
{
	rt_include_module(target, obj);

	return obj;
}

rt_value __cdecl rt_module_included(rt_value obj, size_t argc, rt_value target)
{
	printf("Included module %s in %s\n", rt_string_to_cstr(rt_inspect(obj)), rt_string_to_cstr(rt_inspect(target)));

	return obj;
}

rt_value __cdecl rt_module_include(rt_value obj, size_t argc, ...)
{
	rt_value args[argc];

	va_list _args;
	va_start(_args, argc);

	for(int i = argc - 1; i >= 0; i--)
		args[i] = va_arg(_args, rt_value);

	va_end(_args);

	for(int i = 0; i < argc; i++)
	{
		RT_CALL_CSTR(args[i], "append_features", 1, obj);
		RT_CALL_CSTR(args[i], "included", 1, obj);
	}

	return obj;
}

void rt_module_init(void)
{
	rt_define_method(rt_Module, rt_symbol_from_cstr("to_s"), (rt_compiled_block_t)rt_module_to_s);
	rt_define_method(rt_Module, rt_symbol_from_cstr("append_features"), (rt_compiled_block_t)rt_module_append_features);
	rt_define_method(rt_Module, rt_symbol_from_cstr("include"), (rt_compiled_block_t)rt_module_include);
	rt_define_method(rt_Module, rt_symbol_from_cstr("included"), (rt_compiled_block_t)rt_module_included);
}
