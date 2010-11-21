#pragma once
#include "common.hpp"
#include "memory-pool.hpp"
#include "message.hpp"
#include "map.hpp"
#include "simple-list.hpp"
#include "parser/parser.hpp"

struct exception_block;

namespace Mirb
{
	namespace Tree
	{
		class Scope;
	};
	
	class Block
	{
		public:
			Tree::Scope *scope;
			rt_compiled_block_t compiled; // A pointer to a compiled function.
			Symbol *name; // The name of this block.
			
			Vector<struct exception_block *> exception_blocks;
			void **break_targets;
			size_t local_storage;
			void *epilog;
			
			Vector<Block *> blocks; // A list of child blocks so the GC won't free them.
	};
	
	class BlockMapFunctions:
		public MapFunctions<Symbol *, Block *>
	{
		public:
			static Block *invalid_value()
			{
				return 0;
			}
	};

	typedef Map<Symbol *, Block *, GC, BlockMapFunctions> BlockMap;
};
