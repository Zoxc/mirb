#include "../memory-pool.hpp"
#include "block.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		Block::Block() :
			current_exception_block(0),
			current_exception_block_id(-1),
			self_var(0)
		{
			#ifdef DEBUG
				label_count = 0;
			#endif
		}		
	};
};
