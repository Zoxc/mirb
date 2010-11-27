#include "value.hpp"
#include "classes/object.hpp"

namespace Mirb
{
	namespace Value
	{
		template<> bool of_type<Mirb::Object>(value_t value)
		{
			switch(Value::type(value))
			{
				case Main:
				case Class:
				case IClass:
				case Module:
				case Object:
				case Symbol:
				case String:
				case Array:
				case Proc:
				case Exception:
					return true;

				default:
					return false;
			}
		}
	
		template<> bool of_type<Mirb::Class>(value_t value)
		{
			switch(Value::type(value))
			{
				case Class:
				case IClass:
				case Module:
					return true;

				default:
					return false;
			}
		}

		template<> bool of_type<Mirb::Symbol>(value_t value)
		{
			return Value::type(value) == Symbol;
		}

		bool by_value(value_t value)
		{
			if (value & 1)
				return true;
			else if (value <= value_highest)
			{
				switch(value)
				{
					case value_true:
					case value_false:
					case value_nil:
						return true;

					default:
						Mirb::debug_fail("Unknown literal type");
				}
			}
			else
				return false;
		}

		Type type(value_t value)
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
				return (Type)(uint16_t)cast<Mirb::Object>(value)->type; // TODO: Remove the typecast when everything is migrated
		}
	};
};
