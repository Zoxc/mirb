#include <algorithm>
#include "../generic/memory-pool.hpp"
#include "block.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		BasicBlock::BasicBlock(MemoryPool &memory_pool, Block &block) : block(block), pred_blocks(1, memory_pool), branch_block(0)
		{
			#ifdef DEBUG
				id = block.basic_block_count++;
			#endif
		}
		
		size_t Block::stack_alloc(size_t size)
		{
			size_t result = stack_heap;
			stack_heap += size;

			stack_heap_size = std::max(stack_heap, stack_heap_size);

			return result;
		}
		
		void Block::stack_free(size_t address)
		{
			stack_heap = address;
		}

		Block::Block(MemoryPool &memory_pool, Tree::Scope *scope) :
			scope(scope),
			memory_pool(memory_pool),
			var_count(scope->variable_list.size())
		{
			initialize();
		}
		
		Block::Block(MemoryPool &memory_pool) :
			scope(0),
			memory_pool(memory_pool),
			var_count(0)
		{
			initialize();
		}
		
		void Block::initialize()
		{
			current_exception_block = 0;
			current_exception_block_id = -1;
			heap_array_var = 0;
			heap_var = 0;
			self_var = 0;
			stack_heap = 0;
			stack_heap_size = 0;

			#ifdef DEBUG
				basic_block_count = 0;
			#endif
		}
	};
};
