#pragma once
#include "../../value.hpp"

namespace Mirb
{
	class Symbol;
	
	#define mirb_compiled_block(name) value_t __fastcall name(Symbol *method_name, value_t method_module, value_t obj, value_t block, size_t argc, value_t argv[])

	typedef value_t (__fastcall *compiled_block_t)(Symbol *method_name, value_t method_module, value_t obj, value_t block, size_t argc, value_t argv[]);
};
