#include "../classes.h"
#include "../runtime.h"
#include "../constant.h"
#include "symbol.h"
#include "string.h"

rt_value rt_Object;

rt_compiled_block(rt_object_tap)
{
	RT_CALL_CSTR(block, "call", 1, &obj);
	return obj;
}

rt_compiled_block(rt_Object_allocate)
{
	rt_value result = rt_alloc(sizeof(struct rt_object));

	RT_COMMON(result)->flags = C_OBJECT;
	RT_COMMON(result)->class_of = obj;
	RT_OBJECT(result)->vars = 0;

	return result;
}

rt_compiled_block(rt_object_inspect)
{
    return RT_CALL_CSTR(obj, "to_s", 0, 0);
}

rt_compiled_block(rt_object_to_s)
{
	rt_value c = rt_real_class_of(obj);
	rt_value name = rt_object_get_var(c, rt_symbol_from_cstr("__classname__"));

	if(rt_test(name))
	{
		rt_value result = rt_string_from_cstr("#<");

		rt_concat_string(result, name);
		rt_concat_string(result, rt_string_from_cstr(":0x"));
		rt_concat_string(result, rt_string_from_hex(obj));
		rt_concat_string(result, rt_string_from_cstr(">"));

		return result;
	}
	else
	{
		rt_value result = rt_string_from_cstr("#<0x");

		rt_concat_string(result, rt_string_from_hex(obj));
		rt_concat_string(result, rt_string_from_cstr(">"));

		return result;
	}
}

rt_compiled_block(rt_object_dummy)
{
	return RT_NIL;
}

rt_compiled_block(rt_object_equal)
{
	return RT_BOOL(obj == RT_ARG(0));
}

void rt_object_init(void)
{
	rt_define_method(rt_Object, rt_symbol_from_cstr("initialize"), rt_object_dummy);
    rt_define_method(rt_Object, rt_symbol_from_cstr("inspect"), rt_object_inspect);
    rt_define_method(rt_Object, rt_symbol_from_cstr("to_s"), rt_object_to_s);
    rt_define_method(rt_Object, rt_symbol_from_cstr("tap"), rt_object_tap);
    rt_define_method(rt_Object, rt_symbol_from_cstr("equal?"), rt_object_equal);
    rt_define_method(rt_Object, rt_symbol_from_cstr("eql?"), rt_object_equal);
    rt_define_method(rt_Object, rt_symbol_from_cstr("=="), rt_object_equal);
    rt_define_method(rt_Object, rt_symbol_from_cstr("==="), rt_object_equal);

	rt_define_singleton_method(rt_Object, rt_symbol_from_cstr("allocate"), rt_Object_allocate);
}
