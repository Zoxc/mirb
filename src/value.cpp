#include "value.hpp"
#include "classes.hpp"
#include "document.hpp"
#include "tree/tree.hpp"
#include "runtime.hpp"
#include "collector-common.hpp"

namespace Mirb
{
	struct VerifyFunc
	{
		void operator()(const ValueStorage &storage)
		{
			storage.array->assert_valid_light();
		}

		void operator()(const CharArray &string)
		{
			char_t *data = Accesser::CharArray::data(string);

			if(data && !Accesser::CharArray::static_data(string))
				(&VariableBlock::from_memory((const void *)data))->assert_valid_light();
		}

		template<class T> void operator()(T *value)
		{
			value->assert_valid_light();
		}
	};

	template<Type::Enum type> struct VerifyClass
	{
		typedef void Result;
		typedef typename Type::ToClass<type>::Class Class;

		static void func(value_t value)
		{
			VerifyFunc func;

			if(!Value::immediate(type))
				static_cast<Class *>(value)->mark(func);
		}
	};

	void Value::assert_valid_extended()
	{
		if(prelude_unlikely(context->bootstrap))
			return;

		Type::action<VerifyClass>(type(), this);
	}

	Type::Enum Value::type_table[literal_count];

	bool Value::is_fixnum(value_t value)
	{
		return (size_t)value & fixnum_mask;
	}
		
	void Value::initialize_type_table()
	{
		for(size_t i = 0; i < literal_count; ++i)
		{
			if(i & fixnum_mask)
			{
				type_table[i] = Type::Fixnum;
				continue;
			}

			switch(i)
			{
				case value_nil_num:
				{
					type_table[i] = Type::Nil;
					break;
				}

				case value_false_num:
				{
					type_table[i] = Type::False;
					break;
				}

				case value_true_num:
				{
					type_table[i] = Type::True;
					break;
				}

				default:
				{
					type_table[i] = Type::None;
					break;
				}
			}
		}
	}

	void Value::initialize_class_table()
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

	bool Value::test()
	{
		return ((size_t)this & ~(value_nil_num | value_false_num)) != 0;
	}
	
	bool Value::object_ref()
	{
		return ((size_t)this & object_ref_mask) == 0;
	}
	
	Mirb::Class *Value::class_of_literal()
	{
		return context->class_of_table[(size_t)this & literal_mask];
	}

	Type::Enum Value::type()
	{
		if(object_ref())
			return value_type; // Do a simple cast here to avoid stack overflow when debugging is enabled
		else
			return type_table[(size_t)this & literal_mask];
	}
		
	const size_t Value::magic_value = 12345;
		
	#ifdef DEBUG
		const Value::data_field Value::mark_list = &Value::mark_data;
		const Value::data_field Value::thread_list = &Value::thread_data;
		value_t * const Value::list_end = (value_t *)0xF0;
	#else
		const Value::data_field Value::mark_list = &Value::data;
		const Value::data_field Value::thread_list = &Value::data;
		value_t * const Value::list_end = nullptr;
	#endif
		
	void Value::setup()
	{
		marked = false;

		#ifdef DEBUG
			alive = true;
			magic = magic_value;
			refs = nullptr;
			mark_data = list_end;
			thread_data = list_end;
		#else
			data = list_end;
		#endif
				
		#ifdef VALGRIND
			Collector::heap_list.append(this);
		#endif
	}

	void Value::assert_valid_base()
	{
		mirb_debug(mirb_debug_assert(magic == magic_value));
		mirb_debug_assert(value_type != Type::None && value_type != Type::FreeBlock);
	}

		void Value::assert_alive()
	{
		assert_valid_base();

		mirb_debug(mirb_debug_assert(alive));
	}

	void assert_valid_extended();

	void Value::assert_valid_light()
	{
		if(object_ref())
		{
			assert_alive();
			mirb_debug_assert(marked == false);
			mirb_debug_assert((this->*mark_list) == Value::list_end);
			mirb_debug_assert((this->*thread_list) == Value::list_end);
		}
	}

	void Value::assert_valid()
	{
		#ifdef DEBUG
			assert_valid_light();
				
			#ifdef DEBUG_MEMORY
				if(object_ref())
					assert_valid_extended();
			#endif
		#endif
	}
	
	bool Value::kind_of(Class *klass)
	{
		return subclass_of(klass, internal_class_of(this));
	}
	
	Value::Value(const Value &other) : value_type(other.value_type), hashed(other.hashed), flag(other.flag), flag2(other.flag2)
	{
		setup();
	}

	Value::Value(Type::Enum type) : value_type(type), hashed(false), flag(false)
	{
		setup();
	}

	value_t Value::dup(value_t obj)
	{
		CharArray obj_value = inspect(class_of(obj));

		raise(context->type_error, "Unable to duplicate instances of " + obj_value);
	}

	template<Type::Enum type> struct HashValue
	{
		typedef size_t Result;
		typedef typename Type::ToClass<type>::Class Class;

		static size_t func(value_t value)
		{
			return static_cast<Class *>(value)->hash();
		}
	};

	size_t hash_value(value_t value)
	{
		value->assert_valid();

		return Type::action<HashValue>(value->type(), value);
	}
};
