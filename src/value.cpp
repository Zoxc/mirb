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
			for(size_t i = 0; i < literal_count; ++i)
			{
				if(i & fixnum_mask)
				{
					type_table[i] = Fixnum;
					class_of_table[i] = Mirb::Fixnum::class_ref;
					continue;
				}

				switch(i)
				{
					case value_nil_num:
					{
						type_table[i] = Nil;
						class_of_table[i] = NilClass::class_ref;
						break;
					}

					case value_false_num:
					{
						type_table[i] = False;
						class_of_table[i] = FalseClass::class_ref;
						break;
					}

					case value_true_num:
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

		bool test(value_t value)
		{
			return ((size_t)value & ~(value_nil_num | value_false_num)) != 0;
		}
		
		bool object_ref(value_t value)
		{
			return ((size_t)value & object_ref_mask) == 0;
		}
		
		value_t class_of_literal(value_t value)
		{
			return class_of_table[(size_t)value & literal_mask];
		}

		Type type(value_t value)
		{
			if(object_ref(value))
				return ((Mirb::Object *)value)->get_type(); // Do a simple cast here to avoid stack overflow when debugging is enabled
			else
				return type_table[(size_t)value & literal_mask];
		}
	};
};
