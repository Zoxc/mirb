#include "support.h"
#include "classes.h"
#include "constant.h"
#include "classes/string.h"
#include "classes/symbol.h"
#include "classes/proc.h"

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

void __stdcall rt_support_seal_upval(rt_upval_t *upval)
{
	printf("sealing upval with value %s\n", rt_string_to_cstr(rt_inspect(*(upval->val.upval))));

	upval->val.local = *(upval->val.upval);
	upval->sealed = true;
}

rt_value rt_support_interpolate(size_t argc, ...)
{
	rt_value args[argc];

	va_list _args;
	va_start(_args, argc);

	for(int i = argc - 1; i >= 0; i--)
		args[i] = va_arg(_args, rt_value);

	va_end(_args);

	size_t length = 0;

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
		size_t length = RT_STRING(args[i])->length;

		memcpy(current, RT_STRING(args[i])->string, length);

		current += length;
	}

	*current = 0;

	return rt_string_from_raw_str(new_str, length);
}
