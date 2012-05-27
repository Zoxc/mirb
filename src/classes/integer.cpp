#include "integer.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Integer::to_i(value_t obj)
	{
		return obj;
	}
	
	value_t Integer::pos(value_t obj)
	{
		return obj;
	}
	
	void Integer::initialize()
	{
		method<Arg::Self<Arg::Value>, &pos>(context->integer_class, "+@");
		method<Arg::Self<Arg::Value>, &to_i>(context->integer_class, "to_i");
	}
};

