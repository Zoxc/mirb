#include "../memory-pool.hpp"
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
		
		template<typename T> struct Use
		{
			template<typename lambda> static void func(lambda func, T &opcode)
			{
				opcode.use([&](Tree::Variable *var) { func(var); });
			}
		};

		template<typename T> struct Def
		{
			template<typename lambda> static void func(lambda func, T &opcode)
			{
				opcode.def([&](Tree::Variable *var) { func(var); });
			}
		};
		
		void BasicBlock::prepare_liveness(BitSetWrapper<MemoryPool> &w, size_t var_count, size_t &loc)
		{
			this->var_count = var_count;
			in_work_list = false;
			loc_start = loc;

			in = w.create_clean();
			out = w.create_clean();
			def_bits = w.create_clean();
			use_bits = w.create_clean();

			for(auto i = opcodes.begin(); i != opcodes.end(); ++i)
			{
				i().virtual_do<Def>([&](Tree::Variable *var) {
					var->range.update_start(loc);
					
					w.set(def_bits, var->index);
				});

				i().virtual_do<Use>([&](Tree::Variable *var) {
					var->range.stop = loc - 1;
					
					if(!w.get(def_bits, var->index))
						w.set(use_bits, var->index);
				});

				++loc;
			}

			loc_stop = loc - 1;
		}
		
		void BasicBlock::update_ranges(BitSetWrapper<MemoryPool> &w, Tree::Scope *scope)
		{
			size_t var_count = scope->variable_list.size();

			for(size_t i = 0; i < var_count; ++i)
			{
				if(w.get(in, i))
					scope->variable_list[i]->range.update_start(loc_start);
					
				if(w.get(out, i))
					scope->variable_list[i]->range.update_stop(loc_stop);
			}
		}

		void Block::analyse_liveness(MemoryPool &memory_pool, Tree::Scope *scope)
		{
			BitSetWrapper<MemoryPool> w(memory_pool, scope->variable_list.size());

			size_t loc = 1;

			for(auto i = basic_blocks.begin(); i != basic_blocks.end(); ++i)
				i().prepare_liveness(w, scope->variable_list.size(), loc);

			List<BasicBlock, BasicBlock, &BasicBlock::work_list_entry> work_list;

			work_list.append(epilog);
			epilog->in_work_list = true;
			
			while(!work_list.empty())
			{
				BasicBlock &block = *work_list.first;
				work_list.remove(&block);
				block.in_work_list = false;
				
				bit_set_t in_ = block.in;
				bit_set_t out_ = block.out;

				// block.out is set to the union of all sucessor blocks' in sets

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
				
				// calculate the new in state
				
				block.in = w.set_union(block.use_bits, w.set_difference(block.out, block.def_bits));

				// add predecessor blocks to the work list if the state has mutated

				if(!w.equal(in_, block.in) || !w.equal(out_, block.out))
				{
					for(auto i = block.pred_blocks.begin(); i != block.pred_blocks.end(); ++i)
					{
						if(!i()->in_work_list)
						{
							work_list.append(*i);
							i()->in_work_list = true;
						}
					}
				}

			}

			for(auto i = basic_blocks.begin(); i != basic_blocks.end(); ++i)
				i().update_ranges(w, scope);
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
