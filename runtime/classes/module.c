#include "../classes.h"
#include "../runtime.h"
#include "../constant.h"
#include "symbol.h"
#include "string.h"

rt_value rt_Module;

rt_value rt_module_to_s(rt_value obj, size_t argc)
{
	rt_value name = rt_object_get_var(obj, rt_symbol_from_cstr("__classname__"));

	if(rt_test(name))
		return name;

	return rt_object_to_s(obj, 0);
}

void rt_module_init(void)
{
	rt_define_method(rt_Module, rt_symbol_from_cstr("to_s"), (rt_compiled_block_t)rt_module_to_s);
}
