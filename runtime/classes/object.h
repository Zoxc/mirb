#pragma once
#include "../../globals.h"
#include "../runtime.h"

struct rt_object {
	struct rt_common common;
	hash_t(rt_hash) *vars;
};

#define RT_OBJECT(value) ((struct rt_object *)(value))

extern rt_value rt_Object;

static inline rt_value rt_alloc_object(rt_value obj)
{
	rt_value result = rt_alloc(sizeof(struct rt_object));

	RT_COMMON(result)->flags = C_OBJECT;
	RT_COMMON(result)->class_of = obj;
	RT_OBJECT(result)->vars = 0;

	return result;
}

static inline hash_t(rt_hash) *rt_get_vars(rt_value object)
{
	if(!RT_OBJECT(object)->vars)
		RT_OBJECT(object)->vars = hash_init(rt_hash);

	return RT_OBJECT(object)->vars;
}

rt_compiled_block(rt_Object_allocate);

rt_compiled_block(rt_object_dummy);
rt_compiled_block(rt_object_inspect);
rt_compiled_block(rt_object_to_s);

void rt_object_init(void);
