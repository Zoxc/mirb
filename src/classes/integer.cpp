#include "integer.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Integer::to_i(value_t obj)
	{
		return obj;
	}

	void Integer::initialize()
	{
		method<Arg::Self>(context->integer_class, "to_i", &to_i);
	}
};

