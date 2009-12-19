#include "../classes.h"
#include "../runtime.h"
#include "../constant.h"
#include "symbol.h"
#include "string.h"

rt_value rt_Class;

rt_compiled_block(rt_class_to_s)
{
	rt_value name = rt_object_get_var(obj, rt_symbol_from_cstr("__classname__"));

	if(rt_test(name))
		return name;
	else if(RT_COMMON(obj)->flags & RT_CLASS_SINGLETON)
	{
		rt_value real = rt_object_get_var(obj, rt_symbol_from_cstr("__attached__"));

		real = rt_call(real, "inspect", 0, 0);

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

rt_compiled_block(rt_class_superclass)
{
	rt_value super = rt_real_class(RT_CLASS(obj)->super);

	if(super)
		return super;
	else
		return RT_NIL;
}

rt_compiled_block(rt_class_new)
{
	rt_value result = rt_call(obj, "allocate", 0, 0);

	rt_call(result, "initialize", argc, argv);

	return result;
}

void rt_class_init(void)
{
	rt_define_method(rt_Class, "to_s", rt_class_to_s);
	rt_define_method(rt_Class, "superclass", rt_class_superclass);
	rt_define_method(rt_Class, "new", rt_class_new);
}
