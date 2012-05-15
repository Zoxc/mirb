#include "value-map.hpp"
#include "collector.hpp"
#include "context.hpp"
#include "runtime.hpp"

namespace Mirb
{
	value_t ValueMapData::call(value_t obj, value_t *key)
	{
		return Mirb::call_argv(obj, context->syms.equal, value_nil, 1, key);
	}
	
	value_t ValueMapData::raise()
	{
		Mirb::raise(context->runtime_error, "Recursive function calls");
		return 0;
	}
};

