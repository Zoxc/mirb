#pragma once
#include "../../globals.hpp"
#include "../runtime.hpp"
#include "object.hpp"

#define RT_CLASS_SINGLETON RT_USER_FLAG(0)

struct rt_class {
	struct rt_object object;
	rt_value super;
	hash_t(rt_methods) *methods;
};

#define RT_CLASS(value) ((struct rt_class *)(value))

extern rt_value rt_Class;

static inline hash_t(rt_methods) *rt_get_methods(rt_value object)
{
	if(!RT_CLASS(object)->methods)
		RT_CLASS(object)->methods = hash_init(rt_methods);

	return RT_CLASS(object)->methods;
}

void rt_class_init(void);
