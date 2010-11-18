#pragma once
#include "common.hpp"
#include "memory-pool.hpp"
#include "../../runtime/runtime.hpp"

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
			static rt_compiled_block_t defered_compile(Block *block);
	};
};
