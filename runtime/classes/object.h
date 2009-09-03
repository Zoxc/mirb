#pragma once
#include "../../globals.h"
#include "../runtime.h"

struct rt_object {
	struct rt_common common;
	khash_t(rt_hash) *vars;
};

#define RT_OBJECT(value) ((struct rt_object *)value)

extern rt_value rt_Object;

static inline khash_t(rt_hash) *rt_get_vars(rt_value object)
{
	if(!RT_OBJECT(object)->vars)
		RT_OBJECT(object)->vars = kh_init(rt_hash);

	return RT_OBJECT(object)->vars;
}

rt_value __cdecl rt_object_allocate(rt_value obj, size_t argc);
rt_value __cdecl rt_object_inspect(rt_value obj, size_t argc);
rt_value __cdecl rt_object_to_s(rt_value obj, size_t argc);

void rt_object_init(void);
