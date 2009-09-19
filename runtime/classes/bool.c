#include "../classes.h"
#include "../runtime.h"
#include "../constant.h"
#include "symbol.h"
#include "string.h"

rt_value rt_NilClass;
rt_value rt_TrueClass;
rt_value rt_FalseClass;

rt_value __stdcall rt_nilclass_to_s(rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	return rt_string_from_cstr("");
}

rt_value __stdcall rt_nilclass_inspect(rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	return rt_string_from_cstr("nil");
}

rt_value __stdcall rt_trueclass_to_s(rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	return rt_string_from_cstr("true");
}

rt_value __stdcall rt_falseclass_to_s(rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	return rt_string_from_cstr("false");
}

void rt_bool_init()
{
	rt_NilClass = rt_define_class(rt_Object, rt_symbol_from_cstr("NilClass"), rt_Object);
	rt_TrueClass = rt_define_class(rt_Object, rt_symbol_from_cstr("TrueClass"), rt_Object);
	rt_FalseClass = rt_define_class(rt_Object, rt_symbol_from_cstr("FalseClass"), rt_Object);

	rt_define_method(rt_NilClass, rt_symbol_from_cstr("inspect"), rt_nilclass_inspect);
	rt_define_method(rt_NilClass, rt_symbol_from_cstr("to_s"), rt_nilclass_to_s);
	rt_define_method(rt_TrueClass, rt_symbol_from_cstr("to_s"), rt_trueclass_to_s);
	rt_define_method(rt_FalseClass, rt_symbol_from_cstr("to_s"), rt_falseclass_to_s);

	rt_const_set(rt_Object, rt_symbol_from_cstr("NIL"), RT_NIL);
	rt_const_set(rt_Object, rt_symbol_from_cstr("TRUE"), RT_TRUE);
	rt_const_set(rt_Object, rt_symbol_from_cstr("FALSE"), RT_FALSE);
}
