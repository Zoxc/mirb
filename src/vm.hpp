#pragma once
#include "common.hpp"
#include "block.hpp"

namespace Mirb
{
	value_t evaluate_block(Block *code, value_t self, Symbol *name, value_t module, value_t block, size_t argc, value_t argv[]);
};

