#include "../memory-pool.hpp"
#include "block.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		BasicBlock::BasicBlock(Block &block) : branch(0)
		{
			#ifdef DEBUG
				id = block.basic_block_count++;
			#endif
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
