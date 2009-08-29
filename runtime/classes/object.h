#pragma once
#include "../../globals.h"
#include "../runtime.h"

struct rt_object {
	struct rt_common common;
	khash_t(rt_hash) *vars;
};

#define RT_OBJECT(value) ((struct rt_object *)value)

extern rt_value rt_Object;

rt_value rt_object_allocate(rt_value obj, size_t argc);
rt_value rt_object_inspect(rt_value obj, size_t argc);
rt_value rt_object_to_s(rt_value obj, size_t argc);

void rt_object_init(void);
