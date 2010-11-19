#pragma once
#include "../globals.hpp"
#include "runtime.hpp"
#include "classes/bool.hpp"
#include "classes/fixnum.hpp"
#include "classes/class.hpp"
#include "classes/object.hpp"

extern rt_value rt_main;

static inline rt_value rt_class_of(rt_value obj)
{
	if(obj & RT_FLAG_FIXNUM)
		return rt_Fixnum;
	else if(obj <= RT_MAX)
	{
		switch(obj)
		{
			case RT_TRUE:
				return rt_TrueClass;

			case RT_FALSE:
				return rt_FalseClass;

			case RT_NIL:
				return rt_NilClass;

			default:
				Mirb::debug_fail("Unknown literal type");
		}
	}
	else
		return RT_COMMON(obj)->class_of;
}

static inline rt_value rt_real_class(rt_value obj)
{
	while(obj && ((RT_COMMON(obj)->flags & RT_CLASS_SINGLETON) || (RT_COMMON(obj)->flags & RT_TYPE_MASK) == C_ICLASS))
		obj = RT_CLASS(obj)->super;

	return obj;
}

static inline rt_value rt_real_class_of(rt_value obj)
{
	return rt_real_class(rt_class_of(obj));
}

static inline void rt_class_set_method(rt_value obj, rt_value name, Mirb::Block *block)
{
	int ret;

	hash_iter_t k = hash_put(rt_methods, rt_get_methods(obj), name, &ret);

	if(!ret)
	{
		k = hash_get(rt_methods, RT_CLASS(obj)->methods, name);

		if (k == hash_end(RT_CLASS(obj)->methods))
			RT_ASSERT(0);
	}

	hash_value(RT_CLASS(obj)->methods, k) = block;
}

static inline Mirb::Block *rt_class_get_method(rt_value obj, rt_value name)
{
	if(!RT_CLASS(obj)->methods)
		return 0;

	hash_iter_t k = hash_get(rt_methods, RT_CLASS(obj)->methods, name);

	if (k == hash_end(RT_CLASS(obj)->methods))
		return 0;

	return hash_value(RT_CLASS(obj)->methods, k);
}

static inline hash_t(rt_hash) *rt_object_get_vars(rt_value object)
{
	if(!RT_COMMON(object)->vars)
		RT_COMMON(object)->vars = hash_init(rt_hash);

	return RT_COMMON(object)->vars;
}

static inline void rt_object_set_var(rt_value obj, rt_value name, rt_value value)
{
	hash_t(rt_hash) *vars = rt_object_get_vars(obj);

    hash_iter_t k = hash_get(rt_hash, vars, name);

    if (k == hash_end(vars))
    {
        int ret;

        k = hash_put(rt_hash, vars, name, &ret);

        RT_ASSERT(ret);
    }

	hash_value(vars, k) = value;
}

static inline rt_value rt_object_get_var(rt_value obj, rt_value name)
{
	hash_t(rt_hash) *vars = rt_object_get_vars(obj);

	hash_iter_t k = hash_get(rt_hash, vars, name);

	if (k == hash_end(vars))
		return RT_NIL;

	return hash_value(vars, k);
}

void rt_setup_classes(void);
void rt_destroy_classes(void);

rt_value rt_define_class_symbol(rt_value obj, rt_value name, rt_value super);
rt_value rt_define_class(rt_value under, const char *name, rt_value super);
rt_value rt_define_module_symbol(rt_value obj, rt_value name);
rt_value rt_define_module(rt_value under, const char *name);

void rt_include_module(rt_value obj, rt_value module);

void rt_define_method(rt_value obj, const char *name, rt_compiled_block_t block);
void rt_define_singleton_method(rt_value obj, const char *name, rt_compiled_block_t block);

void rt_class_name(rt_value obj, rt_value under, rt_value name);
rt_value rt_class_create_unnamed(rt_value super);
rt_value rt_class_create_bare(rt_value super, bool root);
rt_value rt_class_create_singleton(rt_value object, rt_value super);

