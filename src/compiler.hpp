#pragma once
#include "common.hpp"
#include "memory-pool.hpp"

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
	};
};
