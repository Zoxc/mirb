#pragma once
#include "common.hpp"
#include "memory-pool.hpp"
#include "block.hpp"

namespace Mirb
{
	namespace Tree
	{
		class Scope;
	};
	
	class Block;
	class Compiler
	{
		private:
		public:
			static Block *compile(Tree::Scope *scope, MemoryPool &memory_pool);
			static Block *defer(Tree::Scope *scope, MemoryPool &memory_pool);
			static compiled_block_t defered_compile(Block *block);
	};
};
