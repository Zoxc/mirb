#include "value-map.hpp"
#include "collector.hpp"
#include "context.hpp"
#include "runtime.hpp"

namespace Mirb
{
	value_t ValueMapData::call(value_t obj, value_t *key)
	{
		return Mirb::call(obj, context->syms.equal, *key);
	}
	
	value_t ValueMapData::raise()
	{
		Mirb::raise(context->runtime_error, "Recursive function calls");
	}
};

