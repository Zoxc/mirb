#include "classes.h"
#include "runtime.h"
#include "symbol.h"
#include "string.h"
#include "constant.h"

rt_value rt_NilClass;
rt_value rt_TrueClass;
rt_value rt_FalseClass;

rt_value rt_NIL;
rt_value rt_TRUE;
rt_value rt_FALSE;

void rt_setup_bool()
{
	rt_NilClass = rt_define_class(rt_Object, rt_symbol_from_cstr("NilClass"), rt_Object);
	rt_TrueClass = rt_define_class(rt_Object, rt_symbol_from_cstr("TrueClass"), rt_Object);
	rt_FalseClass = rt_define_class(rt_Object, rt_symbol_from_cstr("FalseClass"), rt_Object);
}
