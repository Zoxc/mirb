#pragma once
#include "../globals.h"
#include "classes.h"
#include "classes/symbol.h"
#include "classes/string.h"

static inline bool rt_const_defined(rt_value obj, rt_value name)
{
	khash_t(rt_hash) *vars = rt_object_get_vars(obj);

	khiter_t k = kh_get(rt_hash, vars, name);

	return k != kh_end(vars);
}

static inline rt_value rt_const_get(rt_value obj, rt_value name)
{
	while(obj)
	{
		khash_t(rt_hash) *vars = rt_object_get_vars(obj);

		khiter_t k = kh_get(rt_hash, vars, name);

		if(k != kh_end(vars))
			return kh_value(vars, k);

		switch(rt_type(obj))
		{
			case C_CLASS:
			case C_ICLASS:
			case C_MODULE:
				obj = RT_CLASS(obj)->super;
				break;

			default:
				RT_ASSERT(0);
		}
	}

	obj = rt_Object;

	while(obj)
	{
		khash_t(rt_hash) *vars = rt_object_get_vars(obj);

		khiter_t k = kh_get(rt_hash, vars, name);

		if(k != kh_end(vars))
			return kh_value(vars, k);

		switch(rt_type(obj))
		{
			case C_CLASS:
			case C_ICLASS:
			case C_MODULE:
				obj = RT_CLASS(obj)->super;
				break;

			default:
				RT_ASSERT(0);
		}
	}

	printf("Unable to find constant %s on %s.\n", rt_symbol_to_cstr(name), rt_string_to_cstr(rt_inspect(obj)));
	RT_ASSERT(0);
}

static inline void rt_const_set(rt_value obj, rt_value name, rt_value value)
{
	khash_t(rt_hash) *vars = rt_object_get_vars(obj);

	khiter_t k = kh_get(rt_hash, vars, name);

	if (k == kh_end(vars))
		rt_object_set_var(obj, name, value);
	else
	{
		printf("Warning: Reassigning constant %s to ", rt_symbol_to_cstr(name)); rt_print(value); printf("\n");
		kh_value(vars, k) = value;
	}
}

