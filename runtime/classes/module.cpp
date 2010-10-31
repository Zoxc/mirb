#include "../classes.hpp"
#include "../runtime.hpp"
#include "../constant.hpp"
#include "symbol.hpp"
#include "string.hpp"

rt_value rt_Module;

rt_compiled_block(rt_module_to_s)
{
	rt_value name = rt_object_get_var(obj, rt_symbol_from_cstr("__classname__"));

	if(rt_test(name))
		return name;

	return rt_object_to_s(RT_NIL, RT_NIL, obj, 0, 0, 0);
}

rt_compiled_block(rt_module_append_features)
{
	rt_include_module(RT_ARG(0), obj);

	return obj;
}

rt_compiled_block(rt_module_included)
{
	return obj;
}

rt_compiled_block(rt_module_include)
{
	RT_ARG_EACH(i)
	{
		rt_call(argv[i], "append_features", 1, &obj);
		rt_call(argv[i], "included", 1, &obj);
	}

	return obj;
}

void rt_module_init(void)
{
	rt_define_method(rt_Module, "to_s", rt_module_to_s);
	rt_define_method(rt_Module, "append_features", rt_module_append_features);
	rt_define_method(rt_Module, "include", rt_module_include);
	rt_define_method(rt_Module, "included", rt_module_included);
}
