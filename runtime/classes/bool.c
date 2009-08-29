#include "../classes.h"
#include "../runtime.h"
#include "../constant.h"
#include "symbol.h"
#include "string.h"

rt_value rt_NilClass;
rt_value rt_TrueClass;
rt_value rt_FalseClass;

rt_value rt_nilclass_to_s(rt_value obj, size_t argc)
{
	return rt_string_from_cstr("nil");
}

rt_value rt_trueclass_to_s(rt_value obj, size_t argc)
{
	return rt_string_from_cstr("true");
}

rt_value rt_falseclass_to_s(rt_value obj, size_t argc)
{
	return rt_string_from_cstr("false");
}

void rt_bool_init()
{
	rt_NilClass = rt_define_class(rt_Object, rt_symbol_from_cstr("NilClass"), rt_Object);
	rt_TrueClass = rt_define_class(rt_Object, rt_symbol_from_cstr("TrueClass"), rt_Object);
	rt_FalseClass = rt_define_class(rt_Object, rt_symbol_from_cstr("FalseClass"), rt_Object);

	rt_define_method(rt_NilClass, rt_symbol_from_cstr("to_s"), (rt_compiled_block_t)rt_nilclass_to_s);
	rt_define_method(rt_TrueClass, rt_symbol_from_cstr("to_s"), (rt_compiled_block_t)rt_trueclass_to_s);
	rt_define_method(rt_FalseClass, rt_symbol_from_cstr("to_s"), (rt_compiled_block_t)rt_falseclass_to_s);

	rt_const_set(rt_Object, rt_symbol_from_cstr("NIL"), RT_NIL);
	rt_const_set(rt_Object, rt_symbol_from_cstr("TRUE"), RT_TRUE);
	rt_const_set(rt_Object, rt_symbol_from_cstr("FALSE"), RT_FALSE);
}
