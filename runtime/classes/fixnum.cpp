#include "../classes.hpp"
#include "../runtime.hpp"
#include "fixnum.hpp"
#include "symbol.hpp"
#include "string.hpp"

rt_value rt_Fixnum;

rt_compiled_block(rt_fixnum_to_s)
{
	char buffer[15];

	sprintf(buffer, "%d", RT_FIX2INT(obj));

	return rt_string_from_cstr(buffer);
}

rt_compiled_block(rt_fixnum_add)
{
	return RT_INT2FIX(RT_FIX2INT(obj) + RT_FIX2INT(RT_ARG(0)));
}

rt_compiled_block(rt_fixnum_sub)
{
	return RT_INT2FIX(RT_FIX2INT(obj) - RT_FIX2INT(RT_ARG(0)));
}

rt_compiled_block(rt_fixnum_mul)
{
	return RT_INT2FIX(RT_FIX2INT(obj) * RT_FIX2INT(RT_ARG(0)));
}

rt_compiled_block(rt_fixnum_div)
{
	return RT_INT2FIX(RT_FIX2INT(obj) / RT_FIX2INT(RT_ARG(0)));
}

void rt_fixnum_init()
{
	rt_Fixnum = rt_define_class(rt_Object, "Fixnum", rt_Object);

	rt_define_method(rt_Fixnum, "to_s", rt_fixnum_to_s);

	rt_define_method(rt_Fixnum, "+", rt_fixnum_add);
	rt_define_method(rt_Fixnum, "-", rt_fixnum_sub);
	rt_define_method(rt_Fixnum, "*", rt_fixnum_mul);
	rt_define_method(rt_Fixnum, "/", rt_fixnum_div);
}
