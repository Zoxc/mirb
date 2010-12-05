#pragma once
#include "../../value.hpp"

namespace Mirb
{
	class Symbol;
	
	typedef value_t (__fastcall *compiled_block_t)(value_t block, value_t method_module, value_t obj, Symbol *method_name, size_t argc, value_t argv[]);
};
