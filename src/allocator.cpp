#include "allocator.hpp"
#include "collector.hpp"

namespace Mirb
{
	Tuple *AllocatorPrivate::allocate_tuple(size_t size)
	{
		return &Collector::allocate_tuple(size);
	}
};
