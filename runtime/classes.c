#include "classes.h"
#include "runtime.h"
#include "symbols.h"

rt_value rt_Class;
rt_value rt_Module;
rt_value rt_Object;
rt_value rt_Symbol;

rt_value rt_class_create_bare(rt_value super)
{
	rt_value c = rt_alloc(sizeof(struct rt_class));

	RT_COMMON(c)->flags = C_CLASS;
	RT_COMMON(c)->class_of = rt_Class;
	RT_CLASS(c)->super = super;
	RT_CLASS(c)->methods = kh_init(rt_hash);

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

rt_value rt_class_create(rt_value name, rt_value super)
{
	rt_value c = rt_class_create_bare(super);
	rt_class_create_singleton(c, RT_COMMON(super)->class_of);

	return c;
}

