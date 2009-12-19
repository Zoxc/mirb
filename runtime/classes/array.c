#include "../classes.h"
#include "../runtime.h"
#include "symbol.h"
#include "array.h"
#include "string.h"
#include "proc.h"

rt_value rt_Array;

rt_value rt_array_from_raw(rt_value *data, size_t length)
{
	rt_value array = rt_alloc(sizeof(struct rt_array));

	RT_COMMON(array)->flags = C_ARRAY;
	RT_COMMON(array)->class_of = rt_Array;

	RT_ARRAY(array)->data.n = length;
	RT_ARRAY(array)->data.m = length;
	RT_ARRAY(array)->data.a = (rt_value*)malloc(length * sizeof(rt_value));

	memcpy(RT_ARRAY(array)->data.a, data, length * sizeof(rt_value));

	return array;
}

/*
 * Array
 */

rt_compiled_block(rt_Array_allocate)
{
	rt_value array = rt_alloc(sizeof(struct rt_array));

	RT_COMMON(array)->flags = C_ARRAY;
	RT_COMMON(array)->class_of = rt_Array;

	kv_init(RT_ARRAY(array)->data);

	return array;
}

rt_compiled_block(rt_array_push)
{
	RT_ARG_EACH(i)
	{
		kv_push(rt_value, RT_ARRAY(obj)->data, argv[i]);
	}

	return obj;
}

rt_compiled_block(rt_array_pop)
{
	if(kv_size(RT_ARRAY(obj)->data))
		return kv_pop(RT_ARRAY(obj)->data);
	else
		return RT_NIL;
}

rt_compiled_block(rt_array_inspect)
{
	rt_value result = rt_string_from_cstr("[");

	for(size_t i = 0; i < kv_size(RT_ARRAY(obj)->data); i++)
	{
		rt_concat_string(result, rt_call(kv_A(RT_ARRAY(obj)->data, i), "inspect", 0, 0));

		if(i != kv_size(RT_ARRAY(obj)->data) - 1)
			rt_concat_string(result, rt_string_from_cstr(", "));
	}

	rt_concat_string(result, rt_string_from_cstr("]"));

	return result;
}

rt_compiled_block(rt_array_length)
{
	return RT_INT2FIX(kv_size(RT_ARRAY(obj)->data));
}

rt_compiled_block(rt_array_each)
{
	for(size_t i = 0; i < kv_size(RT_ARRAY(obj)->data); i++)
	{
		rt_call_proc(block, RT_NIL, 1, &kv_A(RT_ARRAY(obj)->data, i));
	}

	return obj;
}

void rt_array_init(void)
{
	rt_Array = rt_define_class(rt_Object, "Array", rt_Object);

	rt_define_singleton_method(rt_Array, "allocate", rt_Array_allocate);

	rt_define_method(rt_Array, "push", rt_array_push);
	rt_define_method(rt_Array, "pop", rt_array_pop);
	rt_define_method(rt_Array, "length", rt_array_length);
	rt_define_method(rt_Array, "inspect", rt_array_inspect);
	rt_define_method(rt_Array, "each", rt_array_each);
}

