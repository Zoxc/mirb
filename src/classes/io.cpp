#include "io.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	void IO::initialize()
	{
		context->io_class = define_class("IO", context->object_class);
	}
};

