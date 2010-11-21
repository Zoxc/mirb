#pragma once
#include "../../value.hpp"

namespace Mirb
{
	class Symbol;
	
	typedef Value (__fastcall *compiled_block_t)(Symbol *method_name, Value method_module, Value obj, Value block, size_t argc, Value argv[]);
};
