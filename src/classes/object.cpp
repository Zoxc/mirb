#include "object.hpp"
#include "class.hpp"
#include "../gc.hpp"

namespace Mirb
{
	value_t Object::class_ref;

	value_t Object::allocate()
	{
		return auto_cast(new (gc) Object);
	}
};

