#pragma once
#include "../common.hpp"
#include "../simple-list.hpp"
#include "opcodes.hpp"

namespace Mirb
{
	class Block;
	
	namespace CodeGen
	{
		class Block
		{
			public:
				Mirb::Block *final;
				void *prolog; // The point after the prolog of the block.
				Label *epilog; // The end of the block
				
				#ifdef DEBUG
					size_t label_count; // Nicer label labeling...
				#endif
				
				Block();
				
				SimpleList<Opcode> opcodes; // A linked list of opcodes
		};
	};
};
