#pragma once
#include "../globals.h"
#include "runtime.h"

#define RT_CLASS_SINGLETON RT_USER_FLAG(0)

struct rt_object {
	struct rt_common common;
	khash_t(rt_hash) *vars;
};

struct rt_class {
	struct rt_object object;
	rt_value super;
	khash_t(rt_block) *methods;
};

#define RT_CLASS(value) ((struct rt_class *)value)
#define RT_OBJECT(value) ((struct rt_object *)value)

extern rt_value rt_main;
extern rt_value rt_Class;
extern rt_value rt_Module;
extern rt_value rt_Object;

extern khash_t(rt_hash) *object_var_hashes;

static inline void rt_class_set_method(rt_value obj, rt_value name, rt_compiled_block_t block)
{
	int ret;

	khiter_t k = kh_put(rt_block, RT_CLASS(obj)->methods, name, &ret);

	assert(ret);

	kh_value(RT_CLASS(obj)->methods, k) = block;
}

static inline rt_compiled_block_t rt_class_get_method(rt_value obj, rt_value name)
{
	khiter_t k = kh_get(rt_block, RT_CLASS(obj)->methods, name);

	if (k == kh_end(RT_CLASS(obj)->methods))
		return 0;

	return kh_value(RT_CLASS(obj)->methods, k);
}

khash_t(rt_hash) *rt_object_get_vars(rt_value obj);

static inline void rt_object_set_var(rt_value obj, rt_value name, rt_value value)
{
	khash_t(rt_hash) *vars = rt_object_get_vars(obj);

	int ret;

	khiter_t k = kh_put(rt_hash, vars, name, &ret);

	assert(ret);

	kh_value(vars, k) = value;
}

static inline rt_type_t rt_object_get_var(rt_value obj, rt_value name)
{
	khash_t(rt_hash) *vars = rt_object_get_vars(obj);

	khiter_t k = kh_get(rt_hash, vars, name);

	if (k == kh_end(vars))
		return RT_NIL;

	return kh_value(vars, k);
}

void rt_class_name(rt_value obj, rt_value under, rt_value name);
rt_value rt_class_create_unnamed(rt_value super);
rt_value rt_class_create_bare(rt_value super);
rt_value rt_class_create_singleton(rt_value object, rt_value super);
