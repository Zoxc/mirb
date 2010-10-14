#include "../classes.h"
#include "../runtime.h"
#include "../constant.h"
#include "symbol.h"
#include "string.h"

rt_value rt_NilClass;
rt_value rt_TrueClass;
rt_value rt_FalseClass;

rt_compiled_block(rt_nilclass_to_s)
{
	return rt_string_from_cstr("");
}

rt_compiled_block(rt_nilclass_inspect)
{
	return rt_string_from_cstr("nil");
}

rt_compiled_block(rt_trueclass_to_s)
{
	return rt_string_from_cstr("true");
}

rt_compiled_block(rt_falseclass_to_s)
{
	return rt_string_from_cstr("false");
}

void rt_bool_init()
{
	rt_NilClass = rt_define_class(rt_Object, "NilClass", rt_Object);
	rt_TrueClass = rt_define_class(rt_Object, "TrueClass", rt_Object);
	rt_FalseClass = rt_define_class(rt_Object, "FalseClass", rt_Object);

	rt_define_method(rt_NilClass, "inspect", rt_nilclass_inspect);
	rt_define_method(rt_NilClass, "to_s", rt_nilclass_to_s);
	rt_define_method(rt_TrueClass, "to_s", rt_trueclass_to_s);
	rt_define_method(rt_FalseClass, "to_s", rt_falseclass_to_s);

	rt_const_set(rt_Object, rt_symbol_from_cstr("NIL"), RT_NIL);
	rt_const_set(rt_Object, rt_symbol_from_cstr("TRUE"), RT_TRUE);
	rt_const_set(rt_Object, rt_symbol_from_cstr("FALSE"), RT_FALSE);
}
