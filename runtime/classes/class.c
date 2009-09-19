#include "../classes.h"
#include "../runtime.h"
#include "../constant.h"
#include "symbol.h"
#include "string.h"

rt_value rt_Class;

rt_value __stdcall rt_class_to_s(rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	rt_value name = rt_object_get_var(obj, rt_symbol_from_cstr("__classname__"));

	if(rt_test(name))
		return name;
	else if(RT_COMMON(obj)->flags & RT_CLASS_SINGLETON)
	{
		rt_value real = rt_object_get_var(obj, rt_symbol_from_cstr("__attached__"));

		real = RT_CALL_CSTR(real, "inspect", 0, 0);

		rt_value result = rt_string_from_cstr("#<Class:");
		rt_concat_string(result, real);
		rt_concat_string(result, rt_string_from_cstr(">"));

		return result;
	}
	else
	{
		rt_value result = rt_string_from_cstr("#<Class:0x");

		rt_concat_string(result, rt_string_from_hex(obj));
		rt_concat_string(result, rt_string_from_cstr(">"));

		return result;
	}
}

rt_value __stdcall rt_class_superclass(rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	rt_value super = rt_real_class(RT_CLASS(obj)->super);

	if(super)
		return super;
	else
		return RT_NIL;
}

rt_value __stdcall rt_class_new(rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	rt_value result = RT_CALL_CSTR(obj, "allocate", 0, 0);

	RT_CALL_CSTR(result, "initialize", argc, argv);

	return result;
}

void rt_class_init(void)
{
	rt_define_method(rt_Class, rt_symbol_from_cstr("to_s"), rt_class_to_s);
	rt_define_method(rt_Class, rt_symbol_from_cstr("superclass"), rt_class_superclass);
	rt_define_method(rt_Class, rt_symbol_from_cstr("new"), rt_class_new);
}
