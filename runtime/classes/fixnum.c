#include "../classes.h"
#include "../runtime.h"
#include "fixnum.h"
#include "symbol.h"
#include "string.h"

rt_value rt_Fixnum;
rt_value rt_TrueClass;
rt_value rt_FalseClass;

rt_value rt_fixnum_to_s(rt_value obj, size_t argc)
{
	char buffer[15];

	sprintf(buffer, "%d", RT_FIX2INT(obj));

	return rt_string_from_cstr(buffer);
}

rt_value rt_fixnum_add(rt_value obj, size_t argc, rt_value other)
{
	return RT_INT2FIX(RT_FIX2INT(obj) + RT_FIX2INT(other));
}

rt_value rt_fixnum_sub(rt_value obj, size_t argc, rt_value other)
{
	return RT_INT2FIX(RT_FIX2INT(obj) - RT_FIX2INT(other));
}

rt_value rt_fixnum_mul(rt_value obj, size_t argc, rt_value other)
{
	return RT_INT2FIX(RT_FIX2INT(obj) * RT_FIX2INT(other));
}

rt_value rt_fixnum_div(rt_value obj, size_t argc, rt_value other)
{
	return RT_INT2FIX(RT_FIX2INT(obj) / RT_FIX2INT(other));
}

void rt_fixnum_init()
{
	rt_Fixnum = rt_define_class(rt_Object, rt_symbol_from_cstr("Fixnum"), rt_Object);

	rt_define_method(rt_Fixnum, rt_symbol_from_cstr("to_s"), (rt_compiled_block_t)rt_fixnum_to_s);

	rt_define_method(rt_Fixnum, rt_symbol_from_cstr("+"), (rt_compiled_block_t)rt_fixnum_add);
	rt_define_method(rt_Fixnum, rt_symbol_from_cstr("-"), (rt_compiled_block_t)rt_fixnum_sub);
	rt_define_method(rt_Fixnum, rt_symbol_from_cstr("*"), (rt_compiled_block_t)rt_fixnum_mul);
	rt_define_method(rt_Fixnum, rt_symbol_from_cstr("/"), (rt_compiled_block_t)rt_fixnum_div);
}
