#include "enumerable.hpp"
#include "../classes/fixnum.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	void Enumerable::initialize()
	{
		context->enumerable_module = define_module("Enumerable");
	}
};
