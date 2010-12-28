#pragma once
#include "common.hpp"
#include "generic/memory-pool.hpp"
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
			static Block::compiled_t defered_compile(Block *block);
	};
};
