#include "block.hpp"

namespace Mirb
{
	const size_t no_var = -1;
	
	Block::Block() :
		heap_array_var(no_var),
		heap_var(no_var),
		self_var(no_var)
	{}	
}