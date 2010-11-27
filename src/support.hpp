#pragma once
#include "common.hpp"
#include "block.hpp"

namespace Mirb
{
	namespace Support
	{
		value_t create_closure(Block *block, value_t self, size_t argc, value_t *argv[]);
		value_t create_array(size_t argc, value_t argv[]);
		value_t *create_heap(size_t bytes);
		value_t interpolate(size_t argc, value_t argv[]);

		value_t get_const(value_t obj, Symbol *name);
		void set_const(value_t obj, Symbol *name, value_t value);

		value_t get_ivar(value_t obj, Symbol *name);
		void set_ivar(value_t obj, Symbol *name, value_t value);
		
		value_t define_string(const char *string);
		value_t define_class(value_t obj, Symbol *name, value_t super);
		value_t define_module(value_t obj, Symbol *name);
		void define_method(value_t obj, Symbol *name, Block *block);
		
		value_t call(Symbol *method_name, value_t obj, value_t block, size_t argc, value_t argv[]);
		value_t super(Symbol *method_name, value_t method_module, value_t obj, value_t block, size_t argc, value_t argv[]);
	};
};
