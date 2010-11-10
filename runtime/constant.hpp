#pragma once
#include "../globals.hpp"
#include "classes.hpp"
#include "classes/symbol.hpp"
#include "classes/string.hpp"

static inline bool rt_const_defined(rt_value obj, rt_value name)
{
	hash_t(rt_hash) *vars = rt_object_get_vars(obj);

	hash_iter_t i = hash_get(rt_hash, vars, name);

	return i != hash_end(vars);
}

static inline rt_value rt_const_get(rt_value obj, rt_value name)
{
	while(obj)
	{
		hash_t(rt_hash) *vars = rt_object_get_vars(obj);

		hash_iter_t i = hash_get(rt_hash, vars, name);

		if(i != hash_end(vars))
			return hash_value(vars, i);

		switch(rt_type(obj))
		{
			case C_CLASS:
			case C_ICLASS:
			case C_MODULE:
				obj = RT_CLASS(obj)->super;
				break;

			default:
				Mirb::debug_fail("Wrong object type");
		}
	}

	obj = rt_Object;

	while(obj)
	{
		hash_t(rt_hash) *vars = rt_object_get_vars(obj);

		hash_iter_t i = hash_get(rt_hash, vars, name);

		if(i != hash_end(vars))
			return hash_value(vars, i);

		switch(rt_type(obj))
		{
			case C_CLASS:
			case C_ICLASS:
			case C_MODULE:
				obj = RT_CLASS(obj)->super;
				break;

			default:
				Mirb::debug_fail("Wrong object type");
		}
	}

	printf("Unable to find constant %s on %s.\n", rt_symbol_to_cstr(name), rt_string_to_cstr(rt_inspect(obj)));
	Mirb::debug_fail("Unable to find constant");
}

static inline void rt_const_set(rt_value obj, rt_value name, rt_value value)
{
	hash_t(rt_hash) *vars = rt_object_get_vars(obj);

	hash_iter_t i = hash_get(rt_hash, vars, name);

	if (i == hash_end(vars))
		rt_object_set_var(obj, name, value);
	else
	{
		printf("Warning: Reassigning constant %s to ", rt_symbol_to_cstr(name)); rt_print(value); printf("\n");
		hash_value(vars, i) = value;
	}
}

