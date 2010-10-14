#include "../classes.hpp"
#include "../runtime.hpp"
#include "../constant.hpp"
#include "symbol.hpp"
#include "string.hpp"

rt_value rt_Object;

rt_compiled_block(rt_object_tap)
{
	rt_call(block, "call", 1, &obj);
	return obj;
}

rt_compiled_block(rt_Object_allocate)
{
	return rt_alloc_object(obj);
}

rt_compiled_block(rt_object_inspect)
{
	return rt_call(obj, "to_s", 0, 0);
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
	rt_define_method(rt_Object, "initialize", rt_object_dummy);
	rt_define_method(rt_Object, "inspect", rt_object_inspect);
	rt_define_method(rt_Object, "to_s", rt_object_to_s);
	rt_define_method(rt_Object, "tap", rt_object_tap);
	rt_define_method(rt_Object, "equal?", rt_object_equal);
	rt_define_method(rt_Object, "eql?", rt_object_equal);
	rt_define_method(rt_Object, "==", rt_object_equal);
	rt_define_method(rt_Object, "===", rt_object_equal);

	rt_define_singleton_method(rt_Object, "allocate", rt_Object_allocate);
}
