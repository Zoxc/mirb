#pragma once
#include "../../globals.h"
#include "../runtime.h"
#include "object.h"

#define RT_CLASS_SINGLETON RT_USER_FLAG(0)

struct rt_class {
	struct rt_object object;
	rt_value super;
	khash_t(rt_block) *methods;
};

#define RT_CLASS(value) ((struct rt_class *)value)

extern rt_value rt_Class;

static inline khash_t(rt_block) *rt_get_methods(rt_value object)
{
	if(!RT_CLASS(object)->methods)
		RT_CLASS(object)->methods = kh_init(rt_block);

	return RT_CLASS(object)->methods;
}

void rt_class_init(void);
