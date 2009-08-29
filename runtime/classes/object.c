#include "../classes.h"
#include "../runtime.h"
#include "../constant.h"
#include "symbol.h"
#include "string.h"

rt_value rt_Object;

rt_value rt_object_tap(rt_value obj, size_t argc)
{
	return obj;
}

rt_value rt_object_proc(rt_value obj, size_t argc)
{
	rt_value block;

	__asm__("" : "=c" (block));

	if(block)
		return block;
	else
		return RT_NIL;
}

rt_value rt_object_allocate(rt_value obj, size_t argc)
{
	rt_value result = rt_alloc(sizeof(struct rt_object));

	RT_COMMON(result)->flags = C_OBJECT;
	RT_COMMON(result)->class_of = obj;
	RT_OBJECT(result)->vars = 0;

	return result;
}

rt_value rt_object_inspect(rt_value obj, size_t argc)
{
    return RT_CALL_CSTR(obj, "to_s", 0);
}

rt_value rt_object_to_s(rt_value obj, size_t argc)
{
	rt_value c = rt_real_class_of(obj);
	rt_value name = rt_object_get_var(c, rt_symbol_from_cstr("__classname__"));

	if(rt_test(name))
	{
		rt_value result = rt_string_from_cstr("#<");

		rt_string_concat(result, 1, name);
		rt_string_concat(result, 1, rt_string_from_cstr(":0x"));
		rt_string_concat(result, 1, rt_string_from_hex(obj));
		rt_string_concat(result, 1, rt_string_from_cstr(">"));

		return result;
	}
	else
	{
		rt_value result = rt_string_from_cstr("#<0x");

		rt_string_concat(result, 1, rt_string_from_hex(obj));
		rt_string_concat(result, 1, rt_string_from_cstr(">"));

		return result;
	}
}

void rt_object_init(void)
{
    rt_define_method(rt_Object, rt_symbol_from_cstr("inspect"), (rt_compiled_block_t)rt_object_inspect);
    rt_define_method(rt_Object, rt_symbol_from_cstr("to_s"), (rt_compiled_block_t)rt_object_to_s);
    rt_define_method(rt_Object, rt_symbol_from_cstr("tap"), (rt_compiled_block_t)rt_object_tap);
    rt_define_method(rt_Object, rt_symbol_from_cstr("proc"), (rt_compiled_block_t)rt_object_proc);

	rt_define_singleton_method(rt_Object, rt_symbol_from_cstr("allocate"), (rt_compiled_block_t)rt_object_allocate);
}
