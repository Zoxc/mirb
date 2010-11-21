#pragma once
#include "common.hpp"
#include "block.hpp"

namespace Mirb
{
	namespace Support
	{
		Value create_closure(Block *block, Value self, size_t argc, Value *argv[]);
		Value create_array(size_t argc, Value argv[]);
		Value *create_heap(size_t bytes);
		Value interpolate(size_t argc, Value argv[]);

		Value get_const(Value obj, Symbol *name);
		void set_const(Value obj, Symbol *name, Value value);

		Value get_ivar(Value obj, Symbol *name);
		void set_ivar(Value obj, Symbol *name, Value value);
		
		Value define_string(const char *string);
		Value define_class(Value obj, Value name, Value super);
		Value define_module(Value obj, Value name);
		void define_method(Value obj, Value name, Block *block);
		
		Value call(Symbol *method_name, Value obj, Value block, size_t argc, Value argv[]);
		Value super(Symbol *method_name, Value method_module, Value obj, Value block, size_t argc, Value argv[]);
	};
};
