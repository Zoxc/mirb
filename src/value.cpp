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
		
		void initialize_type_table()
		{
			for(size_t i = 0; i < literal_count; ++i)
			{
				if(i & fixnum_mask)
				{
					type_table[i] = Fixnum;
					continue;
				}

				switch(i)
				{
					case value_nil_num:
					{
						type_table[i] = Nil;
						break;
					}

					case value_false_num:
					{
						type_table[i] = False;
						break;
					}

					case value_true_num:
					{
						type_table[i] = True;
						break;
					}

					default:
					{
						type_table[i] = None;
						break;
					}
				}
			}
		}

		void initialize_class_table()
		{
			for(size_t i = 0; i < literal_count; ++i)
			{
				if(i & fixnum_mask)
				{
					context->class_of_table[i] = Mirb::context->fixnum_class;
					continue;
				}

				switch(i)
				{
					case value_nil_num:
					{
						context->class_of_table[i] = context->nil_class;
						break;
					}

					case value_false_num:
					{
						context->class_of_table[i] = context->false_class;
						break;
					}

					case value_true_num:
					{
						type_table[i] = True;
						context->class_of_table[i] = context->true_class;
						break;
					}

					default:
					{
						context->class_of_table[i] = nullptr;
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
			return auto_cast(context->class_of_table[(size_t)value & literal_mask]);
		}

		Type type(value_t value)
		{
			if(object_ref(value))
				return value->get_type(); // Do a simple cast here to avoid stack overflow when debugging is enabled
			else
				return type_table[(size_t)value & literal_mask];
		}
		
		const size_t Header::magic_value = 12345;
	
		Header::Header(Type type) : type(type), marked(false), alive(true), data(nullptr), data2(nullptr)
		{
			#ifdef DEBUG
				magic = magic_value;
				size = 0;
				refs = nullptr;
			#endif
		}

		Type Header::get_type()
		{
			return type;
		}
	
	};
};
