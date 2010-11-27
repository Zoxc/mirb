#pragma once
#include "value.hpp"
#include "block.hpp"

namespace Mirb
{
	class Symbol;

	std::string inspect(value_t obj);
	ValueMap *get_vars(value_t obj);
	value_t get_const(value_t obj, Symbol *name);
	void set_const(value_t obj, Symbol *name, value_t value);
	value_t get_var(value_t obj, Symbol *name);
	void set_var(value_t obj, Symbol *name, value_t value);
	value_t get_ivar(value_t obj, Symbol *name);
	void set_ivar(value_t obj, Symbol *name, value_t value);
	
	Block *get_method(value_t obj, Symbol *name);
	void set_method(value_t obj, Symbol *name, Block *method);
};

