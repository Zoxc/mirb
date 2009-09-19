#include "../classes.h"
#include "../runtime.h"
#include "../constant.h"
#include "symbol.h"
#include "string.h"

rt_value rt_Object;

rt_value __stdcall rt_object_tap(rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	return obj;
}

rt_value __stdcall rt_Object_allocate(rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	rt_value result = rt_alloc(sizeof(struct rt_object));

	RT_COMMON(result)->flags = C_OBJECT;
	RT_COMMON(result)->class_of = obj;
	RT_OBJECT(result)->vars = 0;

	return result;
}

rt_value __stdcall rt_object_inspect(rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
    return RT_CALL_CSTR(obj, "to_s", 0, 0);
}

rt_value __stdcall rt_object_to_s(rt_value obj, rt_value block, size_t argc, rt_value argv[])
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

rt_value __stdcall rt_object_dummy(rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	return RT_NIL;
}

void rt_object_init(void)
{
	rt_define_method(rt_Object, rt_symbol_from_cstr("initialize"), rt_object_dummy);
    rt_define_method(rt_Object, rt_symbol_from_cstr("inspect"), rt_object_inspect);
    rt_define_method(rt_Object, rt_symbol_from_cstr("to_s"), rt_object_to_s);
    rt_define_method(rt_Object, rt_symbol_from_cstr("tap"), rt_object_tap);

	rt_define_singleton_method(rt_Object, rt_symbol_from_cstr("allocate"), rt_Object_allocate);
}
