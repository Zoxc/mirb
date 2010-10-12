#include "support.h"
#include "classes.h"
#include "constant.h"
#include "exceptions.h"
#include "classes/string.h"
#include "classes/symbol.h"
#include "classes/proc.h"
#include "classes/array.h"

void *__stdcall rt_support_alloc_scope(size_t size)
{
	return (void *)rt_alloc(size);
}

rt_value __stdcall rt_support_define_string(const char* string)
{
	return rt_string_from_cstr(string);
}

#ifndef WIN_SEH
	void __stdcall rt_support_push_frame(struct rt_frame* frame)
	{
		frame->prev = rt_current_frame;
		rt_current_frame = frame;
	}

	void __stdcall rt_support_set_frame(struct rt_frame* frame)
	{
		rt_current_frame = frame;
	}
#endif

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

rt_value __cdecl rt_support_interpolate(size_t argc, rt_value argv[])
{
	size_t length = 0;

	RT_ARG_EACH(i)
	{
		if(rt_type(argv[i]) != C_STRING)
			argv[i] = rt_call(argv[i], "to_s", 0, 0);

		length += RT_STRING(argv[i])->length;
	}

	char *new_str = (char *)rt_alloc_data(length + 1);
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
	RT_COMMON(array)->vars = 0;

	RT_ARRAY(array)->data.size = argc;
	RT_ARRAY(array)->data.max = argc;
	RT_ARRAY(array)->data.array = (rt_value *)rt_alloc(argc * sizeof(rt_value));

	RT_ARG_EACH_RAW(i)
	{
		RT_ARRAY(array)->data.array[i] = RT_ARG(i);
	}

	return array;
}
