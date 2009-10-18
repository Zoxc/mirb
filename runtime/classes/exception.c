#include "../classes.h"
#include "../runtime.h"
#include "exception.h"
#include "string.h"
#include "symbol.h"

rt_value rt_Exception;

/*
 * Exception
 */

rt_value __stdcall rt_Exception_allocate(rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	rt_value exception = rt_alloc(sizeof(struct rt_exception));

	RT_COMMON(exception)->flags = C_EXCEPTION;
	RT_COMMON(exception)->class_of = rt_Exception;

	RT_EXCEPTION(exception)->message = RT_NIL;
	RT_EXCEPTION(exception)->backtrace = RT_NIL;

	return exception;
}

rt_value __stdcall rt_exception_to_s(rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	return RT_EXCEPTION(obj)->message;
}

rt_value __stdcall rt_exception_initialize(rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	RT_EXCEPTION(obj)->message = RT_ARG(0);

	return obj;
}

void rt_exception_init(void)
{
	rt_Exception = rt_define_class(rt_Object, rt_symbol_from_cstr("Exception"), rt_Object);

	rt_define_singleton_method(rt_Exception, rt_symbol_from_cstr("allocate"), rt_Exception_allocate);

	rt_define_method(rt_Exception, rt_symbol_from_cstr("initialize"), rt_exception_initialize);
	rt_define_method(rt_Exception, rt_symbol_from_cstr("message"), rt_exception_to_s);
	rt_define_method(rt_Exception, rt_symbol_from_cstr("to_s"), rt_exception_to_s);
}

