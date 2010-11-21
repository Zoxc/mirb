#include "value.hpp"
#include "classes/object.hpp"

namespace Mirb
{
	Value::Type Value::type()
	{
		if (raw & 1)
			return Fixnum;
		else if (raw <= value_highest)
		{
			switch(raw)
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
			return (Type)object->type;
	}
};

