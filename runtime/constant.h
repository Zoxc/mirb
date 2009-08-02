#pragma once
#include "../globals.h"
#include "classes.h"

static inline bool rt_const_defined(rt_value obj, rt_value name)
{
	khash_t(rt_hash) *vars = rt_object_get_vars(obj);

	khiter_t k = kh_get(rt_hash, vars, name);

	return k != kh_end(vars);
}

static inline rt_value rt_const_get(rt_value obj, rt_value name)
{
	khash_t(rt_hash) *vars = rt_object_get_vars(obj);

	khiter_t k = kh_get(rt_hash, vars, name);

	assert(k != kh_end(vars));

	return kh_value(vars, k);
}

static inline void rt_const_set(rt_value obj, rt_value name, rt_value value)
{
	khash_t(rt_hash) *vars = rt_object_get_vars(obj);

	khiter_t k = kh_get(rt_hash, vars, name);

	if (k == kh_end(vars))
		rt_object_set_var(obj, name, value);
	else
	{
		printf("Warning: Reassigning constant %s to", rt_symbol_to_cstr(name)); rt_print(value); printf("\n");
		kh_value(vars, k) = value;
	}
}

