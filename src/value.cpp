#include "value.hpp"
#include "classes/object.hpp"

namespace Mirb
{
	Value::Type Value::type(value_t value)
	{
		if (value & 1)
			return Fixnum;
		else if (value <= value_highest)
		{
			switch(value)
			{
				case value_true:
					return True;

				case value_false:
					return False;

				case value_nil:
					return Nil;

				default:
					Mirb::debug_fail("Unknown literal type");
			}
		}
		else
			return ((Mirb::Object *)value)->type;
	}
};

