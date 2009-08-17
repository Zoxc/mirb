#include "support.h"
#include "string.h"
#include "symbol.h"
#include "classes.h"
#include "constant.h"
#include "proc.h"

rt_value __stdcall rt_support_define_string(const char* string)
{
	return rt_string_from_cstr(string);
}

rt_value __stdcall rt_support_get_const(rt_value obj, rt_value name)
{
	printf("Looking up constant %s in %s\n", rt_symbol_to_cstr(name), rt_string_to_cstr(rt_inspect(obj)));

	if(obj == rt_main)
		obj = rt_Object;

	return rt_const_get(obj, name);
}

void __stdcall rt_support_set_const(rt_value obj, rt_value name, rt_value value)
{
	printf("Setting constant %s in %s to %s\n", rt_symbol_to_cstr(name), rt_string_to_cstr(rt_inspect(obj)), rt_string_to_cstr(rt_inspect(value)));

	if(obj == rt_main)
		obj = rt_Object;

	rt_const_set(obj, name, value);
}

rt_value __stdcall rt_support_define_class(rt_value under, rt_value name, rt_value super)
{
	if(under == rt_main)
		under = rt_Object;

	return rt_define_class(under, name, super);
}

void __stdcall rt_support_define_method(rt_value obj, rt_value name, rt_compiled_block_t block)
{
	if(obj == rt_main)
		obj = rt_Object;

	rt_define_method(obj, name, block);
}

rt_value rt_support_closure(rt_compiled_block_t block, unsigned int argc, ...)
{
	rt_value closure = (rt_value)malloc(sizeof(struct rt_proc));

	RT_COMMON(closure)->flags = C_PROC;
	RT_COMMON(closure)->class_of = rt_Proc;
	RT_PROC(closure)->block = block;
	RT_PROC(closure)->upval_count = argc;
	RT_PROC(closure)->upvals = malloc(sizeof(rt_upval_t *) * argc);

	printf("making a closure off block %x\n", block);

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

void __stdcall rt_support_seal_upval(rt_upval_t *upval)
{
	printf("sealing upval with value %s\n", rt_string_to_cstr(rt_inspect(*(upval->val.upval))));

	upval->val.local = *(upval->val.upval);
	upval->sealed = true;
}

rt_value rt_support_interpolate(unsigned int argc, ...)
{
	rt_value args[argc];

	va_list _args;
	va_start(_args, argc);

	for(int i = argc - 1; i >= 0; i--)
		args[i] = va_arg(_args, rt_value);

	va_end(_args);

	unsigned int length = 0;

	for(int i = 0; i < argc; i++)
	{
		if(rt_type(args[i]) != C_STRING)
			args[i] = RT_CALL_CSTR(args[i], "to_s", 0);

		length += RT_STRING(args[i])->length;
	}

	char *new_str = malloc(length + 1);
	char *current = new_str;

	for(int i = 0; i < argc; i++)
	{
		unsigned int length = RT_STRING(args[i])->length;

		memcpy(current, RT_STRING(args[i])->string, length);

		current += length;
	}

	*current = 0;

	return rt_string_from_raw_str(new_str, length);
}
