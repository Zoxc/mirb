#include "classes.h"
#include "runtime.h"
#include "symbol.h"
#include "string.h"
#include "constant.h"

rt_value rt_main;
rt_value rt_Class;
rt_value rt_Module;
rt_value rt_Object;

khash_t(rt_hash) *object_var_hashes;

static inline bool rt_object_has_vars(rt_value obj)
{
	if (obj & RT_FLAG_FIXNUM)
		return false;
	else if (obj <= RT_MAIN)
		return false;
	else
		switch(RT_COMMON(obj)->flags & RT_TYPE_MASK)
		{
			case C_CLASS:
			case C_MODULE:
			case C_OBJECT:
				return true;

			default:
				return false;
		}
}

khash_t(rt_hash) *rt_object_get_vars(rt_value obj)
{
	if (rt_object_has_vars(obj))
		return RT_OBJECT(obj)->vars;
	else if(obj == RT_MAIN)
		return RT_OBJECT(rt_Object)->vars;
	else
	{
		khiter_t k = kh_get(rt_hash, object_var_hashes, obj);

		if (k != kh_end(object_var_hashes))
			return (khash_t(rt_hash) *)kh_value(object_var_hashes, k);

		int ret;

		k = kh_put(rt_hash, object_var_hashes, obj, &ret);

		assert(ret);

		khash_t(rt_hash) *hash = kh_init(rt_hash);

		kh_value(object_var_hashes, k) = (rt_value)hash;

		return hash;
	}
}

rt_value rt_class_create_bare(rt_value super)
{
	rt_value c = rt_alloc(sizeof(struct rt_class));

	RT_COMMON(c)->flags = C_CLASS;
	RT_COMMON(c)->class_of = rt_Class;
	RT_OBJECT(c)->vars = kh_init(rt_hash);
	RT_CLASS(c)->super = super;
	RT_CLASS(c)->methods = kh_init(rt_block);

	return c;
}

rt_value rt_class_create_singleton(rt_value object, rt_value super)
{
	rt_value singleton = rt_class_create_bare(super);

	RT_COMMON(singleton)->flags |= RT_CLASS_SINGLETON;
	RT_COMMON(object)->class_of = singleton;

	if (rt_type(object) == C_CLASS)
	{
		RT_COMMON(singleton)->class_of = singleton;

		//if (RT_COMMON(object)->flags & RT_CLASS_SINGLETON)
		//	RT_CLASS(singleton)->super = rt_class_real(RT_CLASS(object)->super)->class_of;
	}

	return singleton;
}

rt_value rt_class_singleton(rt_value object)
{
	if (RT_COMMON(object)->flags & RT_CLASS_SINGLETON)
		return RT_COMMON(object)->class_of;

	return rt_class_create_singleton(object, RT_COMMON(object)->class_of);
}

rt_value rt_class_create_unnamed(rt_value super)
{
	rt_value obj = rt_class_create_bare(super);
	rt_class_create_singleton(obj, RT_COMMON(super)->class_of);

	return obj;
}

void rt_class_name(rt_value obj, rt_value under, rt_value name)
{
	rt_value under_path = rt_object_get_var(under, rt_symbol_from_cstr("__classpath__"));

	char *new_path = malloc(RT_STRING(under)->length + strlen(rt_symbol_to_cstr(name)) + 3);

	if (under == rt_Object)
	{
		strcpy(new_path, rt_symbol_to_cstr(name));
	}
	else
	{
		strcpy(new_path, rt_string_to_cstr(under_path));
		strcat(new_path, "::");
		strcat(new_path, rt_symbol_to_cstr(name));
	}

	rt_object_set_var(obj, rt_symbol_from_cstr("__classname__"), name);
	rt_object_set_var(obj, rt_symbol_from_cstr("__classpath__"), rt_string_from_cstr(new_path));

	rt_const_set(under, name, obj);
}

