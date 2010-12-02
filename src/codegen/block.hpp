#pragma once
#include "../common.hpp"
#include "../generic/memory-pool.hpp"
#include "../generic/list.hpp"
#include "../generic/vector.hpp"
#include "../generic/bit-set-wrapper.hpp"
#include "opcodes.hpp"
#include "../tree/tree.hpp"

struct exception_block;

namespace Mirb
{
	class Block;
	struct ExceptionBlock;
	
	namespace Tree
	{
		class Variable;
		class Scope;
	};
	
	namespace CodeGen
	{
		class Block;
		class BasicBlock;
		
		class BasicBlock
		{
			public:
				BasicBlock(MemoryPool &memory_pool, Block &block);

				#ifdef DEBUG
					size_t id;
				#endif

				Block &block;

				Entry<BasicBlock> entry;
				Entry<BasicBlock> work_list_entry;

				bool in_work_list;
				
				bit_set_t def_bits;
				bit_set_t use_bits;
				
				bit_set_t in;
				bit_set_t out;

				Vector<BasicBlock *, MemoryPool> pred_blocks;

				SimpleList<Opcode> opcodes; // A linked list of opcodes
				
				BasicBlock *next_block;
				BasicBlock *branch_block;

				size_t loc_start;
				size_t loc_stop;

				void *final;
				
				void prepare_liveness(BitSetWrapper<MemoryPool> &w);
				void update_ranges(BitSetWrapper<MemoryPool> &w, size_t var_count);
				
				void next(BasicBlock *block)
				{
					next_block = block;
					block->pred_blocks.push(this);
				}

				void branch(BasicBlock *block)
				{
					branch_block = block;
					block->pred_blocks.push(this);
				}
		};

		class Block
		{
			private:
				void initialize();
			public:
				Mirb::Block *final;
				Tree::Scope *scope;
				BasicBlock *epilog; // The end of the block

				MemoryPool &memory_pool;
				
				/*
				 * Exception related fields
				 */
				ExceptionBlock *current_exception_block;
				size_t current_exception_block_id;

				size_t var_count;

				size_t stack_vars;
				
				bit_set_t used_registers;

				Vector<Tree::Variable *, MemoryPool> variable_list; // A list of all variables in this block. Copied from Tree::Scope::variable_list on creation.
				
				Tree::Variable *heap_array_var;
				Tree::Variable *heap_var;
				Tree::Variable *self_var;
				Tree::Variable *return_var;
				
				#ifdef DEBUG
					size_t basic_block_count; // Nicer basic block labeling...
				#endif
				
				size_t loc;
				
				void analyse_liveness();
				void allocate_registers();
				
				Block(MemoryPool &memory_pool, Tree::Scope *scope);
				Block(MemoryPool &memory_pool);
				
				List<BasicBlock> basic_blocks; // A linked list of basic blocks
		};
	};
};
