#pragma once
#include "common.hpp"
#include "block.hpp"
#include "value.hpp"

namespace Mirb
{
	namespace Support
	{
		value_t create_closure(Block *block, value_t self, Symbol *name, Tuple<Module> *scope, size_t argc, value_t argv[]);
		value_t create_array(size_t argc, value_t argv[]);
		value_t create_hash(size_t argc, value_t argv[]);
		value_t interpolate(size_t argc, value_t argv[], Value::Type type);

		void define_method(Tuple<Module> *scope, Symbol *name, Block *block);
		bool define_singleton_method(Tuple<Module> *scope, value_t obj, Symbol *name, Block *block);
	};
};
