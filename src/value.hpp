#pragma once
#include "type.hpp"

#ifdef VALGRIND
	#include <Prelude/LinkedList.hpp>
#endif

namespace Mirb
{;
	/* Format for tagged pointers
	 *
	 *  1 - fixnum
	 * 00 - object reference, only above 0x4 (0x4 would make the value zero if bit 2 and 3 is cleared)
	 * 10 - literal objects
	 */
	 
	/* Bit layout for literal objects. You can test a value by clearing bit 2 and 3, if the result is 0, it's nil or false.
	 *
	 * 0010 - nil
	 * 0110 - false
	 * 1010 - true
	 * 1110 - undef
	 */
	
	typedef Value *value_t;
	
	const size_t fixnum_mask = 1;

	#define mirb_object_align 4

	const size_t object_ref_mask = mirb_object_align - 1;

	const size_t literal_mask = 0xF;
	const size_t literal_count = (literal_mask + 1);
	
	const size_t value_nil_num = 2;
	const size_t value_false_num = 6;
	const size_t value_true_num = 10;

	const value_t value_nil = (value_t)value_nil_num;
	const value_t value_false = (value_t)value_false_num;
	const value_t value_true = (value_t)value_true_num;
	const value_t value_undef = (value_t)14; // Used to mark words in vectors containing no references.
	
	namespace Arg
	{
		template<class T> class Class;
		class Value;
	};

	class Value
	{
	public:
	private:
		#ifdef DEBUG
			value_t *mark_data;
			value_t *thread_data;
		#else
			value_t *data;
		#endif

		void setup();
	public:
		Value(const Value &other);
		Value(Type::Enum type);

		template<class A> struct ArgumentWrapper
		{
			typedef typename Arg::Class<A> Real;
		};
		
		static const size_t type_bits = 7;

		static_assert(((1 << type_bits) - 1) >= (size_t)Type::Types, "Too few bits for type field");
				
		static const size_t magic_value;
		static value_t * const list_end;
				
		Type::Enum value_type : type_bits;
		bool marked : 1;
		bool hashed : 1;

		// Usable by subclasses
		bool flag : 1; 
		bool flag2 : 1;

		#ifdef DEBUG
			bool alive : 1;
			size_t block_size;
			size_t magic;
			value_t refs;
		#endif

		typedef value_t *Value::*data_field;
				
		static const data_field mark_list;
		static const data_field thread_list;
				
		#ifdef VALGRIND
			LinkedListEntry<Header> entry;
		#endif

		static value_t dup(value_t obj);

		static const bool finalizer = false;

		static size_t hash()
		{
			mirb_runtime_abort("No hash function defined for object");
		}

		bool object_ref();
		Mirb::Class *class_of_literal();
		bool test();
		
		bool kind_of(Class *klass);

		static Type::Enum type_table[literal_count];

		static void initialize_type_table();
		static void initialize_class_table();

		static bool immediate(Type::Enum type)
		{
			switch(type)
			{
				case Type::Fixnum:
				case Type::True:
				case Type::False:
				case Type::Nil:
					return true;
				default:
					return false;
			}
		}

		Type::Enum type();
		
		void assert_valid_base() prelude_nonnull(1);
		void assert_alive() prelude_nonnull(1);
		void assert_valid_extended() prelude_nonnull(1);
		void assert_valid_light() prelude_nonnull(1);
		void assert_valid() prelude_nonnull(1);

		static value_t from_bool(bool value)
		{
			return value ? value_true : value_false;
		}

		static bool is_fixnum(value_t value);
	};
	
	template<> struct Value::ArgumentWrapper<Value>
	{
		typedef Arg::Value Real;
	};
		
	template<Type::Enum type> struct Immediate:
		public Value
	{
		template<typename F> void mark(F) {}

		size_t hash()
		{
			return hash_number((size_t)this); // Note: First bit is constant
		}
	};

	class ImmediateFixnum:
		public Immediate<Type::Fixnum>
	{
	};

	class ImmediateTrue:
		public Immediate<Type::True>
	{
	};

	class ImmediateFalse:
		public Immediate<Type::False>
	{
	};

	class ImmediateNil:
		public Immediate<Type::Nil>
	{
	};
	
	template<class Base> struct OfType
	{
		template<Type::Enum type> struct Test
		{
			typedef bool Result;

			static bool func(bool)
			{
				return Type::DerivedFrom<Base, typename Type::ToClass<type>::Class>::value;
			}
		};
	};
	
	template<class T> bool of_type(value_t obj)
	{
		obj->assert_valid();

		return Type::action<OfType<T>::template Test>(obj->type(), true);
	}
	
	template<class T> void verify(T *value prelude_unused)
	{
		mirb_debug_assert(of_type<T>((value_t)value));
	}
		
	size_t hash_value(value_t value);
		
	void initialize_type_table();
	void initialize_class_table();

	template<typename T> T *cast_null(value_t obj)
	{
		mirb_debug_assert(obj == 0 || of_type<T>(obj));
		return reinterpret_cast<T *>(obj);
	}

	template<typename T> T *cast(value_t obj)
	{
		mirb_debug_assert(of_type<T>(obj));
		return reinterpret_cast<T *>(obj);
	}

	template<typename T> T *try_cast(value_t obj)
	{
		if(of_type<T>(obj))
			return reinterpret_cast<T *>(obj);
		else
			return 0;
	}
};
