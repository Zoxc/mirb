#pragma once
#include "../../value.hpp"

namespace Mirb
{
	class Symbol;
	
	typedef value_t (__fastcall *compiled_block_t)(Symbol *method_name, value_t method_module, value_t obj, value_t block, size_t argc, value_t argv[]);
};
