#include "classes.h"
#include "runtime.h"
#include "symbol.h"
#include "string.h"
#include "constant.h"
#include "proc.h"
#include "x86.h"

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
	unsigned int index;

	__asm__("" : "=a" (index));

	rt_upval_t *upval = upvals[index];

	if(!upval->sealed)
		return *(upval->val.upval);
	else
		return upval->val.local;
}

void __stdcall rt_support_set_upval(rt_upval_t **upvals, rt_value value)
{
	unsigned int index;

	__asm__("" : "=a" (index));

	rt_upval_t *upval = upvals[index];

	if(!upval->sealed)
		*(upval->val.upval) = value;
	else
		upval->val.local = value;
}
