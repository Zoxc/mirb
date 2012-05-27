#include "time.hpp"
#include "float.hpp"
#include "../runtime.hpp"
#include "../platform/platform.hpp"

namespace Mirb
{
	value_t Time::now()
	{
		return Collector::allocate<Float>(Platform::get_time());
	}

	void Time::initialize()
	{
		context->time_class = define_class("Time", context->object_class);

		singleton_method<&now>(context->time_class, "now");
	}
};

