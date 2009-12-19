#pragma once
#include "../classes.h"
#include "../vector.h"

RT_VEC_INIT(rt_value, rt);

struct rt_array {
	struct rt_common common;
	vec_t(rt) data;
};

#define RT_ARRAY(value) ((struct rt_array *)(value))

extern rt_value rt_Array;

rt_value rt_array_from_raw(rt_value *data, size_t length);

void rt_array_init(void);
