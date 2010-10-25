#pragma once
#include "../common.hpp"
#include "../simple-list.hpp"
#include "opcodes.hpp"

namespace Mirb
{
	class Block;
	
	namespace Tree
	{
		class Variable;
	};
	
	namespace CodeGen
	{
		class Block
		{
			public:
				Mirb::Block *final;
				Label *prolog; // The point after the prolog of the block.
				Label *epilog; // The end of the block
				
				/*
				 * Exception related fields
				 */
				struct exception_block *current_exception_block;
				size_t current_exception_block_id;
				
				Tree::Variable *self_var;
				Tree::Variable *return_var;
				
				#ifdef DEBUG
					size_t label_count; // Nicer label labeling...
				#endif
				
				Block();
				
				SimpleList<Opcode> opcodes; // A linked list of opcodes
		};
	};
};
