#include "../memory-pool.hpp"
#include "../tree/tree.hpp"
#include "block.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		BasicBlock::BasicBlock(MemoryPool &memory_pool, Block &block) : pred_blocks(1, memory_pool), branch_block(0)
		{
			#ifdef DEBUG
				id = block.basic_block_count++;
			#endif
		}
		
		void BasicBlock::def(Tree::Variable *var)
		{
			BitSetWrapper<MemoryPool>::set(def_bits, var->index);
		}

		void BasicBlock::use(Tree::Variable *var)
		{
			BitSetWrapper<MemoryPool>::set(use_bits, var->index);
		}

		void BasicBlock::prepare_liveness(BitSetWrapper<MemoryPool> &w, size_t var_count)
		{
			this->var_count = var_count;
			in_work_list = false;

			in = w.create_clean();
			out = w.create_clean();
			def_bits = w.create_clean();
			use_bits = w.create_clean();

			for(auto i = opcodes.begin(); i; ++i)
				i().def_use(*this);
		}

		void Block::analyse_liveness(MemoryPool &memory_pool, Tree::Scope *scope)
		{
			BitSetWrapper<MemoryPool> w(memory_pool, scope->local_vars);

			for(auto i = basic_blocks.begin(); i; ++i)
				i().prepare_liveness(w, scope->local_vars);

			List<BasicBlock, BasicBlock, &BasicBlock::work_list_entry> work_list;

			work_list.append(epilog);
			epilog->in_work_list = true;
			
			while(!work_list.empty())
			{
				BasicBlock &block = *work_list.first;

				bit_set_t in_ = block.in;
				bit_set_t out_ = block.out;

				block.in = w.set_union(block.use_bits, w.set_difference(out_, block.def_bits));

				if(block.next_block)
				{
					block.out = w.dup(block.next_block->in);

					if(block.branch_block)
						w.mod_set_union(block.out, block.branch_block->in);
				}
				else if(block.branch_block)
					block.out = w.dup(block.branch_block->in);
				else
					block.out = w.create_clean();
				
				if(!w.equal(in_, block.in) || !w.equal(out_, block.out))
				{
					for(auto i = block.pred_blocks.begin(); i; ++i)
					{
						if(!i()->in_work_list)
						{
							work_list.append(*i);
							i()->in_work_list = true;
						}
					}
				}

				work_list.remove(&block);
				block.in_work_list = false;
			}
		}

		Block::Block() :
			current_exception_block(0),
			current_exception_block_id(-1),
			self_var(0)
		{
			#ifdef DEBUG
				basic_block_count = 0;
			#endif
		}	
	};
};
