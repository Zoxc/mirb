#pragma once
#include "common.hpp"
#include "memory-pool.hpp"
#include "message.hpp"
#include "simple-list.hpp"
#include "parser/parser.hpp"

namespace Mirb
{
	class Block
	{
		public:
			rt_compiled_block_t compiled; // A pointer to a compiled function.
			Symbol *name; // The name of this block.
			
			Vector<struct exception_block *> exception_blocks;
			void **break_targets;
			size_t local_storage;
			void *epilog;
			
			SimplerEntry<Block> entry;
			SimplerList<Block> blocks; // A list of child blocks so the GC won't free them.
	};
};
