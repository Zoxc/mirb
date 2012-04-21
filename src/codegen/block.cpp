#include <algorithm>
#include "../generic/memory-pool.hpp"
#include "block.hpp"
#include "../vm.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		BasicBlock::BasicBlock(MemoryPool &memory_pool, Block &block) : block(block), branch_block(0)
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

		void Block::finalize()
		{
			size_t size = 0;

			for(auto i = basic_blocks.begin(); i != basic_blocks.end(); ++i)
				size += (size_t)i().opcodes.tellp();

			const char *opcodes = (const char *)malloc(size);
			mirb_runtime_assert(opcodes);
			size_t pos = 0;
			
			for(auto i = basic_blocks.begin(); i != basic_blocks.end(); ++i)
			{
				std::string data = i().opcodes.str();

				i().pos = pos;

				memcpy((void *)&opcodes[pos], data.data(), data.size());

				pos += data.size();
			}
			
			for(auto i = basic_blocks.begin(); i != basic_blocks.end(); ++i)
			{
				for(auto j = i().branches.begin(); j != i().branches.end(); ++j)
				{
					BranchOp *op = (BranchOp *)&opcodes[i().pos + (*j).first];

					op->pos = (*j).second->pos;
				}
			}

			for(auto i = exception_blocks.begin(); i != exception_blocks.end(); ++i)
			{
				if(i()->ensure_label.block)
					i()->ensure_label.address = i()->ensure_label.block->pos;
				else
					i()->ensure_label.address = -1;

				for(auto j = i()->handlers.begin(); j != i()->handlers.end(); ++j)
				{
					switch(j()->type)
					{
						case RuntimeException:
						{
							RuntimeExceptionHandler *handler = (RuntimeExceptionHandler *)*j;

							handler->rescue_label.address = handler->rescue_label.block->pos;

							break;
						}

						case ClassException:
						case FilterException:
						default:
							break;
					}
				}
			}

			final->opcodes = opcodes;
			final->var_words = var_count;
			final->executor = &evaluate_block;
		}
		
		void Block::initialize()
		{
			current_exception_block = 0;
			stack_heap = 0;
			stack_heap_size = 0;

			#ifdef DEBUG
				basic_block_count = 0;
			#endif
		}
	};
};
