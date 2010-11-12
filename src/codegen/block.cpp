#include <algorithm>
#include "../memory-pool.hpp"
#include "../arch/arch.hpp"
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
		
		void BasicBlock::prepare_liveness(BitSetWrapper<MemoryPool> &w, Block *block, size_t &loc)
		{
			in_work_list = false;
			loc_start = loc;

			in = w.create_clean();
			out = w.create_clean();
			def_bits = w.create_clean();
			use_bits = w.create_clean();

			for(auto i = opcodes.begin(); i != opcodes.end(); ++i)
			{
				i().virtual_do<Use>([&](Tree::Variable *var) {
					var->range.stop = loc;
					
					if(!w.get(def_bits, var->index))
						w.set(use_bits, var->index);
				});

				i().virtual_do<Def>([&](Tree::Variable *var) {
					var->range.update_start(loc);
					
					w.set(def_bits, var->index);
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

		void Block::analyse_liveness()
		{
			BitSetWrapper<MemoryPool> w(memory_pool, scope->variable_list.size());

			size_t loc = 0;

			for(auto i = basic_blocks.begin(); i != basic_blocks.end(); ++i)
				i().prepare_liveness(w, this, loc);

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

		void Block::allocate_registers()
		{
			std::vector<Tree::Variable *> variables;

			for(auto i = scope->variable_list.begin(); i != scope->variable_list.end(); ++i)
				if(i()->type != Tree::Variable::Heap)
					variables.push_back(*i);

			// TODO: Sort directly on scope->variable_list

			std::sort(variables.begin(), variables.end(), [](Tree::Variable *a, Tree::Variable *b) { return a->range.start < b->range.start; });

			Tree::Variable *used_reg[Arch::registers] = {0};

			auto get_reg = [&](Tree::Variable *var) -> size_t {
				for(size_t i = 0; i < Arch::registers; ++i)
					if(!used_reg[i])
					{
						used_reg[i] = var;
						return i;
					}

				debug_fail("No more registers!");
			};

			// TODO: Don't use std::vector
			
			std::vector<Tree::Variable *> active;

			auto add_active = [&](Tree::Variable *var) -> void {
				auto pos = std::lower_bound(active.begin(), active.end(), var, [](Tree::Variable *a, Tree::Variable *b) { return a->range.stop > b->range.stop; });

				active.insert(pos, var);
			};

			size_t stack_loc = 0;

			for(auto i = variables.begin(); i != variables.end(); ++i)
			{
				// Expire old intervals

				auto j = active.rbegin();
				
				for(; j != active.rend() && (*j)->range.stop <= (*i)->range.start; ++j)
					used_reg[(*j)->loc] = false;
				
				active.resize(active.size() - (j - active.rbegin()));

				// Assign a location to this interval

				if((*i)->reg)
				{
					// This interval already has a register, make sure it is free

					Tree::Variable *used = used_reg[(*i)->loc];

					if(used)
					{
						std::remove(active.begin(), active.end(), used);
						used->loc = stack_loc++;
						used->reg = false;
					}

					add_active(*i);
				}
				else if(active.size() == Arch::registers)
				{
					// Figure out which interval to spill 

					Tree::Variable *spill = active.front();

					if(spill->range.stop > (*i)->range.stop)
					{
						(*i)->reg = true;
						(*i)->reg = spill->reg;
						spill->loc = stack_loc++;
						spill->reg = false;
						active.erase(active.begin());
						add_active(*i);
					}
					else
						(*i)->loc = stack_loc++;
				}
				else
				{
					(*i)->reg = true;
					(*i)->loc = get_reg(*i);
					add_active(*i);
				}
			}
		}

		Block::Block(MemoryPool &memory_pool, Tree::Scope *scope) :
			scope(scope),
			memory_pool(memory_pool),
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
