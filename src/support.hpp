#pragma once
#include "common.hpp"
#include "block.hpp"
#include "../../runtime/classes.hpp"

namespace Mirb
{
	namespace Support
	{
		rt_value create_closure(Block *block, rt_value self, rt_value method_name, rt_value method_module, size_t argc, rt_value *argv[]);
		
		rt_value define_string(const char *string);
		rt_value define_class(rt_value obj, rt_value name, rt_value super);
		rt_value define_module(rt_value obj, rt_value name);
		void define_method(rt_value obj, rt_value name, Block *block);
		
		rt_value call(Symbol *method_name, rt_value obj, rt_value block, size_t argc, rt_value argv[]);
		rt_value super(Symbol *method_name, rt_value method_module, rt_value obj, rt_value block, size_t argc, rt_value argv[]);
	};
};
