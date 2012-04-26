#include <algorithm>
#include "../generic/memory-pool.hpp"
#include "block.hpp"
#include "../vm.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		BasicBlock::BasicBlock(MemoryPool &memory_pool, Block &block) : branches(memory_pool), source_locs(memory_pool)
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
			self_var(no_var),
			heap_var(no_var),
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
			mirb_debug_assert(final->opcodes == nullptr);
			
			size_t size = 0;
			size_t ranges = 0;

			for(auto i = basic_blocks.begin(); i != basic_blocks.end(); ++i)
			{
				size += (size_t)i().opcodes.tellp();
				ranges += i().source_locs.size();
			}

			final->ranges = (Range *)malloc(ranges * sizeof(Range));
			mirb_runtime_assert(final->ranges != 0);
			const char *opcodes = (const char *)malloc(size);
			mirb_runtime_assert(opcodes != 0);
			size_t pos = 0;
			ranges = 0;
			
			for(auto i = basic_blocks.begin(); i != basic_blocks.end(); ++i)
			{
				std::string data = i().opcodes.str();

				i().pos = pos;
				
				for(auto j = i().source_locs.begin(); j != i().source_locs.end(); ++j, ++ranges)
				{
					final->ranges[ranges] = *j().second;

					final->source_location.set(pos + j().first, &final->ranges[ranges]);
				}

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

			final->scope = nullptr;
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
