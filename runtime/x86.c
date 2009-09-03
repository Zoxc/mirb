#include "classes.h"
#include "runtime.h"
#include "constant.h"
#include "x86.h"
#include "classes/symbol.h"
#include "classes/string.h"
#include "classes/proc.h"

rt_value rt_support_closure(rt_compiled_closure_t block, size_t argc, ...)
{
	rt_value self;

	__asm__("" : "=D" (self));

	rt_value closure = (rt_value)malloc(sizeof(struct rt_proc));

	RT_COMMON(closure)->flags = C_PROC;
	RT_COMMON(closure)->class_of = rt_Proc;
	RT_PROC(closure)->self = self;
	RT_PROC(closure)->closure = block;
	RT_PROC(closure)->upval_count = argc;
	RT_PROC(closure)->upvals = malloc(sizeof(rt_upval_t *) * argc);

	printf("making a closure off block %x on %s\n", (rt_value)block, rt_string_to_cstr(rt_inspect(self)));

	va_list _args;
	va_start(_args, argc);

	for(int i = argc - 1; i >= 0; i--)
	{
		RT_PROC(closure)->upvals[i] = va_arg(_args, rt_upval_t *);
		printf("adding upval with value %s as index %d\n", rt_string_to_cstr(rt_inspect(*RT_PROC(closure)->upvals[i]->val.upval)), (argc - 1) - i);
	}

	va_end(_args);

	return closure;
}

rt_compiled_block_t __cdecl rt_support_lookup_method(rt_value obj)
{
	rt_value method;

	__asm__("" : "=a" (method));
/*
	rt_compiled_block_t result = rt_lookup(obj, method);

	__asm__("jmp %0" : : "r" (result));
*/
	return rt_lookup(obj, method);
}

rt_value __stdcall rt_support_define_class(rt_value name, rt_value super)
{
	rt_value obj;

	__asm__("" : "=D" (obj));

	if(obj == rt_main)
		obj = rt_Object;

	return rt_define_class(obj, name, super);
}

rt_value __stdcall rt_support_define_module(rt_value name)
{
	rt_value obj;

	__asm__("" : "=D" (obj));

	if(obj == rt_main)
		obj = rt_Object;

	return rt_define_module(obj, name);
}

void __stdcall rt_support_define_method(rt_value name, rt_compiled_block_t block)
{
	rt_value obj;

	__asm__("" : "=D" (obj));

	if(obj == rt_main)
		obj = rt_Object;

	rt_define_method(obj, name, block);
}


rt_upval_t *rt_support_upval_create()
{
	rt_value *real;

	__asm__("" : "=a" (real));

	printf("creating upval with value %s\n", rt_string_to_cstr(rt_inspect(*real)));

	rt_upval_t *result = malloc(sizeof(rt_upval_t));

	result->val.upval = real;
	result->sealed = false;

	return result;
}

rt_value __stdcall rt_support_get_upval(rt_upval_t **upvals)
{
	size_t index;

	__asm__("" : "=a" (index));

	rt_upval_t *upval = upvals[index];

	if(!upval->sealed)
		return *(upval->val.upval);
	else
		return upval->val.local;
}

void __stdcall rt_support_set_upval(rt_upval_t **upvals, rt_value value)
{
	size_t index;

	__asm__("" : "=a" (index));

	rt_upval_t *upval = upvals[index];

	if(!upval->sealed)
		*(upval->val.upval) = value;
	else
		upval->val.local = value;
}
