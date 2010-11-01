#pragma once
#include "../common.hpp"
#include "../list.hpp"
#include "opcodes.hpp"

struct exception_block;

namespace Mirb
{
	class Block;
	
	namespace Tree
	{
		class Variable;
	};
	
	namespace CodeGen
	{
		class BasicBlock;
		
		class BasicBlock
		{
			public:
				BasicBlock(Block &block);

				#ifdef DEBUG
					size_t id;
				#endif

				Entry<BasicBlock> entry;

				SimpleList<Opcode> opcodes; // A linked list of opcodes
				
				BasicBlock *next;
				BasicBlock *branch;
		};

		class Block
		{
			public:
				Mirb::Block *final;
				BasicBlock *prolog; // The point after the prolog of the block.
				BasicBlock *epilog; // The end of the block
				
				/*
				 * Exception related fields
				 */
				struct exception_block *current_exception_block;
				size_t current_exception_block_id;
				
				Tree::Variable *self_var;
				Tree::Variable *return_var;
				
				#ifdef DEBUG
					size_t basic_block_count; // Nicer basic block labeling...
				#endif
				
				Block();
				
				List<BasicBlock> basic_blocks; // A linked list of basic blocks
		};
	};
};
