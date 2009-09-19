#include "../classes.h"
#include "../runtime.h"
#include "../constant.h"
#include "symbol.h"
#include "string.h"

rt_value rt_Module;

rt_value __stdcall rt_module_to_s(rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	rt_value name = rt_object_get_var(obj, rt_symbol_from_cstr("__classname__"));

	if(rt_test(name))
		return name;

	return rt_object_to_s(obj, 0, 0, 0);
}

rt_value __stdcall rt_module_append_features(rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	rt_include_module(RT_ARG(0), obj);

	return obj;
}

rt_value __stdcall rt_module_included(rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	return obj;
}

rt_value __stdcall rt_module_include(rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	RT_ARG_EACH(i)
	{
		RT_CALL_CSTR(argv[i], "append_features", 1, &obj);
		RT_CALL_CSTR(argv[i], "included", 1, &obj);
	}

	return obj;
}

void rt_module_init(void)
{
	rt_define_method(rt_Module, rt_symbol_from_cstr("to_s"), rt_module_to_s);
	rt_define_method(rt_Module, rt_symbol_from_cstr("append_features"), rt_module_append_features);
	rt_define_method(rt_Module, rt_symbol_from_cstr("include"), rt_module_include);
	rt_define_method(rt_Module, rt_symbol_from_cstr("included"), rt_module_included);
}
