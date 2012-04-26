#include "value-map.hpp"
#include "collector.hpp"

namespace Mirb
{
	ValueMapFunctions::Pair *ValueMapFunctions::allocate_pair()
	{
		return Collector::allocate<Pair>();
	}
};

