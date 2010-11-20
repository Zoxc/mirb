#pragma once
#include "common.hpp"
#include "block.hpp"
#include "../../runtime/classes.hpp"

namespace Mirb
{
	namespace Support
	{
		rt_value create_closure(Block *block, rt_value self, size_t argc, rt_value *argv[]);
		rt_value create_array(size_t argc, rt_value argv[]);
		rt_value *create_heap(size_t bytes);
		rt_value interpolate(size_t argc, rt_value argv[]);

		rt_value get_const(rt_value obj, Symbol *name);
		void set_const(rt_value obj, Symbol *name, rt_value value);

		rt_value get_ivar(rt_value obj, Symbol *name);
		void set_ivar(rt_value obj, Symbol *name, rt_value value);
		
		rt_value define_string(const char *string);
		rt_value define_class(rt_value obj, rt_value name, rt_value super);
		rt_value define_module(rt_value obj, rt_value name);
		void define_method(rt_value obj, rt_value name, Block *block);
		
		rt_value call(Symbol *method_name, rt_value obj, rt_value block, size_t argc, rt_value argv[]);
		rt_value super(Symbol *method_name, rt_value method_module, rt_value obj, rt_value block, size_t argc, rt_value argv[]);
	};
};
