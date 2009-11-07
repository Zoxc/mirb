#include "support.h"
#include "classes.h"
#include "constant.h"
#include "classes/string.h"
#include "classes/symbol.h"
#include "classes/proc.h"
#include "classes/array.h"

rt_value __stdcall rt_support_define_string(const char* string)
{
	return rt_string_from_cstr(string);
}

rt_value __stdcall rt_support_get_const(rt_value obj, rt_value name)
{
	if(obj == rt_main)
		obj = rt_Object;

	#ifdef DEBUG
		printf("Looking up constant %s in %s\n", rt_symbol_to_cstr(name), rt_string_to_cstr(rt_inspect(obj)));
	#endif

	return rt_const_get(obj, name);
}

void __stdcall rt_support_set_const(rt_value obj, rt_value name, rt_value value)
{
	if(obj == rt_main)
		obj = rt_Object;

	#ifdef DEBUG
		printf("Setting constant %s in %s to %s\n", rt_symbol_to_cstr(name), rt_string_to_cstr(rt_inspect(obj)), rt_string_to_cstr(rt_inspect(value)));
	#endif

	rt_const_set(obj, name, value);
}

void __stdcall rt_support_seal_upval(struct rt_upval *upval)
{
	upval->val.local = *(upval->val.upval);
	upval->sealed = true;
}

rt_value __cdecl rt_support_interpolate(size_t argc, rt_value argv[])
{
	size_t length = 0;

	RT_ARG_EACH(i)
	{
		if(rt_type(argv[i]) != C_STRING)
			argv[i] = RT_CALL_CSTR(argv[i], "to_s", 0, 0);

		length += RT_STRING(argv[i])->length;
	}

	char *new_str = malloc(length + 1);
	char *current = new_str;

	RT_ARG_EACH(i)
	{
		size_t length = RT_STRING(argv[i])->length;

		memcpy(current, RT_STRING(argv[i])->string, length);

		current += length;
	}

	*current = 0;

	return rt_string_from_raw_str(new_str, length);
}

rt_value __cdecl rt_support_array(size_t argc, rt_value argv[])
{
	rt_value array = rt_alloc(sizeof(struct rt_array));

	RT_COMMON(array)->flags = C_ARRAY;
	RT_COMMON(array)->class_of = rt_Array;

	RT_ARRAY(array)->data.n = argc;
	RT_ARRAY(array)->data.m = argc;
	RT_ARRAY(array)->data.a = (rt_value*)malloc(argc * sizeof(rt_value));

	RT_ARG_EACH_RAW(i)
	{
		RT_ARRAY(array)->data.a[i] = RT_ARG(i);
	}

	return array;
}
