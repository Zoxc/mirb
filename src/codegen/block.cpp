#include <algorithm>
#include "../memory-pool.hpp"
#include "../arch/arch.hpp"
#include "../arch/codegen.hpp"
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
		
		template<typename T> struct UseDef
		{
			static void func(BasicBlock &block, T &opcode)
			{
				if(FlushRegisters<T>::value)
				{
					auto var = new (block.block.memory_pool) Tree::Variable(Tree::Variable::Temporary);
					
					var->flags.set<Tree::Variable::FlushCallerSavedRegisters>();

					block.block.scope->variable_list.push(var);
					
					var->range.start = block.block.loc;
					var->range.stop = block.block.loc;
				}

				opcode.use([&](Tree::Variable *var) {
					var->range.stop = block.block.loc;
					
					if(!BitSetWrapper<MemoryPool>::get(block.def_bits, var->index))
						BitSetWrapper<MemoryPool>::set(block.use_bits, var->index);
				});

				opcode.def([&](Tree::Variable *var) {
					var->range.update_start(block.block.loc);
					
					BitSetWrapper<MemoryPool>::set(block.def_bits, var->index);
				});
			}
		};
		
		void BasicBlock::prepare_liveness(BitSetWrapper<MemoryPool> &w)
		{
			in_work_list = false;
			loc_start = block.loc;

			in = w.create_clean();
			out = w.create_clean();
			def_bits = w.create_clean();
			use_bits = w.create_clean();

			for(auto i = opcodes.begin(); i != opcodes.end(); ++i)
			{
				i().virtual_do<UseDef>(*this);

				++block.loc;
			}

			loc_stop = block.loc - 1;
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

			loc = 0;

			for(auto i = basic_blocks.begin(); i != basic_blocks.end(); ++i)
				i().prepare_liveness(w);

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

			std::sort(variables.begin(), variables.end(), [](Tree::Variable *a, Tree::Variable *b) -> bool {
				if(a->range.start < b->range.start)
					return true;
				else if(a->range.start == b->range.start)
					return a->flags.get<Tree::Variable::FlushCallerSavedRegisters>();
				else
					return false; 
			});
			
			BitSetWrapper<MemoryPool> w(memory_pool, Arch::registers);
			Tree::Variable *used_reg[Arch::registers] = {0};

			used_registers = w.create_clean();

			auto get_reg = [&](Tree::Variable *var) -> size_t {
				for(size_t i = 0; i < Arch::registers; ++i)
					if(!used_reg[i])
					{
						used_reg[i] = var;
						size_t real = Arch::Register::to_real(i);
						w.set(used_registers, real);
						return real;
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
			
			auto flush_reg = [&](size_t loc, Tree::Variable *var) -> void {
				Tree::Variable *used = used_reg[loc];

				if(used)
				{
					Tree::Variable *spill = active.front();

					if(spill != used)
					{
						used->loc = spill->loc;
						used_reg[Arch::Register::to_virtual(used->loc)] = used;
					}
					
					spill->loc = stack_loc++;
					spill->flags.clear<Tree::Variable::Register>();

					active.erase(active.begin());
				}

				used_reg[loc] = var;
			};

			for(auto i = variables.begin(); i != variables.end(); ++i)
			{
				// Expire old intervals

				auto j = active.rbegin();
				
				for(; j != active.rend() && (*j)->range.stop <= (*i)->range.start; ++j)
					used_reg[Arch::Register::to_virtual((*j)->loc)] = 0;
				
				active.resize(active.size() - (j - active.rbegin()));

				// Assign a location to this interval

				if((*i)->flags.get<Tree::Variable::FlushCallerSavedRegisters>())
				{
					// Flush all caller saved registers

					std::for_each(Arch::caller_saved, Arch::caller_saved + (sizeof(Arch::caller_saved) / sizeof(size_t)), [&] (size_t reg) {
						flush_reg(Arch::Register::to_virtual(reg), 0);
					});
				}
				else if((*i)->flags.get<Tree::Variable::Register>())
				{
					// Check if this register can be assigned by other variables

					size_t loc = Arch::Register::to_virtual((*i)->loc);

					if(loc != (size_t)Arch::Register::None) 
					{
						// This register can be used by other variables, make sure it is free

						flush_reg(loc, *i);

						add_active(*i);
					}
				}
				else if(active.size() == Arch::registers)
				{
					// Figure out which interval to spill 

					Tree::Variable *spill = active.front();

					if(spill->range.stop > (*i)->range.stop)
					{
						(*i)->flags.set<Tree::Variable::Register>();
						(*i)->loc = spill->loc;
						spill->loc = stack_loc++;
						spill->flags.clear<Tree::Variable::Register>();
						active.erase(active.begin());
						add_active(*i);
						used_reg[Arch::Register::to_virtual((*i)->loc)] = *i;
					}
					else
						(*i)->loc = stack_loc++;
				}
				else
				{
					(*i)->flags.set<Tree::Variable::Register>();
					(*i)->loc = get_reg(*i);
					add_active(*i);
				}
			}

			stack_vars = stack_loc;
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
