#include "integer.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Integer::to_i(value_t obj)
	{
		return obj;
	}
	
	value_t Integer::ord(value_t obj)
	{
		return obj;
	}
	
	void Integer::initialize()
	{
		method<Self<Value>, &ord>(context->integer_class, "ord");
		method<Self<Value>, &to_i>(context->integer_class, "to_i");
	}
};

