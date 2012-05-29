#include "value.hpp"
#include "classes.hpp"
#include "document.hpp"
#include "tree/tree.hpp"
#include "runtime.hpp"
#include "collector-common.hpp"

namespace Mirb
{
	namespace Value
	{
		std::string names[] = {
			"None",
			"FreeBlock",
			"InternalValueMap",
			"InternalStackFrame",
			"InternalTuple",
			"InternalValueTuple",
			"InternalVariableBlock",
			"InternalDocument",
			"InternalBlock",
			"InternalScope",
			"InternalGlobal",
			"Float",
			"Range",
			"Fixnum",
			"TrueClass",
			"FalseClass",
			"NilClass",
			"Class",
			"IClass",
			"Module",
			"Object",
			"Symbol",
			"String",
			"Regexp",
			"Bignum",
			"Array",
			"Hash",
			"Method",
			"Proc",
			"Exception",
			"ReturnException",
			"BreakException",
			"NextException",
			"RedoException",
			"SystemStackError",
			"IO"
		};

		struct VerifyFunc
		{
			void operator()(const ValueStorage &storage)
			{
				assert_valid_light(storage.array);
			}

			void operator()(const CharArray &string)
			{
				char_t *data = Accesser::CharArray::data(string);

				if(data && !Accesser::CharArray::static_data(string))
					assert_valid_light(&VariableBlock::from_memory((const void *)data));
			}

			template<class T> void operator()(T *value)
			{
				assert_valid_light(value);
			}
		};

		template<Value::Type type> struct VerifyClass
		{
			typedef void Result;
			typedef typename Value::TypeClass<type>::Class Class;

			static void func(value_t value)
			{
				VerifyFunc func;

				if(!Value::immediate(type))
					static_cast<Class *>(value)->mark(func);
			}
		};

		void assert_valid_extended(value_t obj)
		{
			if(prelude_unlikely(context->bootstrap))
				return;

			Value::virtual_do<VerifyClass>(Value::type(obj), obj);
		}

		Type type_table[literal_count];

		bool is_fixnum(value_t value)
		{
			return (size_t)value & fixnum_mask;
		}
		
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
		
		Mirb::Class *class_of_literal(value_t value)
		{
			return context->class_of_table[(size_t)value & literal_mask];
		}

		Type type(value_t value)
		{
			if(object_ref(value))
				return value->get_type(); // Do a simple cast here to avoid stack overflow when debugging is enabled
			else
				return type_table[(size_t)value & literal_mask];
		}
		
		const size_t Header::magic_value = 12345;
		
		#ifdef DEBUG
			const Header::data_field Header::mark_list = &Header::mark_data;
			const Header::data_field Header::thread_list = &Header::thread_data;
			value_t * const Header::list_end = (value_t *)0xF0;
		#else
			const Header::data_field Header::mark_list = &Header::data;
			const Header::data_field Header::thread_list = &Header::data;
			value_t * const Header::list_end = nullptr;
		#endif
		
		void Header::setup()
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

		Header::Header(const Header &other) : type(other.type), hashed(other.hashed), flag(other.flag)
		{
			setup();
		}

		Header::Header(Type type) : type(type), hashed(false), flag(false)
		{
			setup();
		}

		Type Header::get_type()
		{
			return type;
		}
		
		value_t Header::dup(value_t obj)
		{
			CharArray obj_value = inspect(class_of(obj));

			raise(context->type_error, "Unable to duplicate instances of " + obj_value);
		}

		template<Type type> struct HashValue
		{
			typedef size_t Result;
			typedef typename TypeClass<type>::Class Class;

			static size_t func(value_t value)
			{
				return static_cast<Class *>(value)->hash();
			}
		};

		size_t hash(value_t value)
		{
			Value::assert_valid(value);

			return virtual_do<HashValue>(type(value), value);
		}

	};
};
