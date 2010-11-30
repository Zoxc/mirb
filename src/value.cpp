#include "value.hpp"
#include "classes/object.hpp"
#include "classes/fixnum.hpp"
#include "classes/nil-class.hpp"
#include "classes/false-class.hpp"
#include "classes/true-class.hpp"

namespace Mirb
{
	namespace Value
	{
		Type type_table[literal_count];
		value_t class_of_table[literal_count];

		void initialize()
		{
			for(value_t i = 0; i < literal_count; ++i)
			{
				if(i & fixnum_mask)
				{
					type_table[i] = Fixnum;
					class_of_table[i] = Mirb::Fixnum::class_ref;
					continue;
				}

				switch(i)
				{
					case value_nil:
					{
						type_table[i] = Nil;
						class_of_table[i] = NilClass::class_ref;
						break;
					}

					case value_false:
					{
						type_table[i] = False;
						class_of_table[i] = FalseClass::class_ref;
						break;
					}

					case value_true:
					{
						type_table[i] = True;
						class_of_table[i] = TrueClass::class_ref;
						break;
					}

					default:
					{
						type_table[i] = None;
						class_of_table[i] = 0;
						break;
					}
				}
			}
		}

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
		
		template<> bool of_type<Mirb::Module>(value_t value)
		{
			switch(Value::type(value))
			{
				case Module:
				case Class:
				case IClass:
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
					return true;

				default:
					return false;
			}
		}
		
		template<> bool of_type<Mirb::Symbol>(value_t value)
		{
			return Value::type(value) == Symbol;
		}
		
		template<> bool of_type<Mirb::String>(value_t value)
		{
			return Value::type(value) == String;
		}
		
		template<> bool of_type<Mirb::Array>(value_t value)
		{
			return Value::type(value) == Array;
		}
		
		template<> bool of_type<Mirb::Exception>(value_t value)
		{
			return Value::type(value) == Exception;
		}
		
		template<> bool of_type<Mirb::Proc>(value_t value)
		{
			return Value::type(value) == Proc;
		}
		
		bool test(value_t value)
		{
			return (value & ~(value_nil | value_false)) != 0;
		}
		
		bool object_ref(value_t value)
		{
			return (value & object_ref_mask) == 0;
		}
		
		value_t class_of_literal(value_t value)
		{
			return class_of_table[value & literal_mask];
		}

		Type type(value_t value)
		{
			if(object_ref(value))
				return ((Mirb::Object *)value)->type; // Do a simple cast here to avoid stack overflow when debugging is enabled
			else
				return type_table[value & literal_mask];
		}
	};
};
