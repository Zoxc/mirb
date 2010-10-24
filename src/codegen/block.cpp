#include "../memory-pool.hpp"
#include "block.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		Block::Block()
		{
			#ifdef DEBUG
				label_count = 0;
			#endif
		}		
	};
};
