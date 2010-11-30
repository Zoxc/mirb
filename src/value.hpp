#pragma once
#include "common.hpp"
#include "gc.hpp"
#include "generic/map.hpp"

namespace Mirb
{
	class Object;
	class Module;
	class Class;
	class Symbol;
	class String;
	class Array;
	class Exception;
	class Proc;

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

	typedef size_t value_t;

	const value_t fixnum_mask = 1;
	const value_t object_ref_mask = 3;
	
	const value_t literal_mask = 0xF;
	const value_t literal_count = literal_mask + 1;
	
	const value_t value_nil = 2;
	const value_t value_false = 6;
	const value_t value_true = 10;
	const value_t value_undef = 14;
	const value_t value_highest = value_undef;

	namespace Value
	{
		enum Type {
			None,
			Fixnum,
			True,
			False,
			Nil,
			Main,
			Class,
			IClass,
			Module,
			Object,
			Symbol,
			String,
			Array,
			Proc,
			Exception
		};

		bool object_ref(value_t value);
		
		value_t class_of_literal(value_t value);

		bool test(value_t value);

		template<class T> bool of_type(value_t value)
		{
			return false;
		}
		
		template<> bool of_type<Mirb::Object>(value_t value);
		template<> bool of_type<Mirb::Module>(value_t value);
		template<> bool of_type<Mirb::Class>(value_t value);
		template<> bool of_type<Mirb::Symbol>(value_t value);
		template<> bool of_type<Mirb::String>(value_t value);
		template<> bool of_type<Mirb::Array>(value_t value);
		template<> bool of_type<Mirb::Exception>(value_t value);
		template<> bool of_type<Mirb::Proc>(value_t value);

		Type type(value_t value);

		void initialize();
	};
	
	union auto_cast
	{
		value_t value;
			
		auto_cast(size_t value) : value(value) {}

		auto_cast(bool value)
		{
			this->value = value ? value_true : value_false;
		}
		
		auto_cast(Object *obj) : value((value_t)obj)
		{
			mirb_debug_assert(Value::of_type<Object>(value));
		}

		auto_cast(Module *obj) : value((value_t)obj)
		{
			mirb_debug_assert(Value::of_type<Module>(value));
		}

		auto_cast(Class *obj) : value((value_t)obj)
		{
			mirb_debug_assert(Value::of_type<Class>(value));
		}
		
		auto_cast(Symbol *obj) : value((value_t)obj)
		{
			mirb_debug_assert(Value::of_type<Symbol>(value));
		}
		
		auto_cast(String *obj) : value((value_t)obj)
		{
			mirb_debug_assert(Value::of_type<String>(value));
		}

		auto_cast(Array *obj) : value((value_t)obj)
		{
			mirb_debug_assert(Value::of_type<Array>(value));
		}
		
		auto_cast(Exception *obj) : value((value_t)obj)
		{
			mirb_debug_assert(Value::of_type<Exception>(value));
		}

		auto_cast(Proc *obj) : value((value_t)obj)
		{
			mirb_debug_assert(Value::of_type<Proc>(value));
		}

		operator value_t() { return value; }
		
		operator Object *()
		{
			mirb_debug_assert(Value::of_type<Object>(value));
			return (Object *)value;
		}

		operator Module *()
		{
			mirb_debug_assert(Value::of_type<Module>(value));
			return (Module *)value;
		}

		operator Class *()
		{
			mirb_debug_assert(Value::of_type<Class>(value));
			return (Class *)value;
		}

		operator Symbol *()
		{
			mirb_debug_assert(Value::of_type<Symbol>(value));
			return (Symbol *)value;
		}

		operator String *()
		{
			mirb_debug_assert(Value::of_type<String>(value));
			return (String *)value;
		}
		
		operator Array *()
		{
			mirb_debug_assert(Value::of_type<Array>(value));
			return (Array *)value;
		}
		
		operator Exception *()
		{
			mirb_debug_assert(Value::of_type<Exception>(value));
			return (Exception *)value;
		}
		
		operator Proc *()
		{
			mirb_debug_assert(Value::of_type<Proc>(value));
			return (Proc *)value;
		}
	};

	template<typename T> T *cast(value_t obj)
	{
		mirb_debug_assert(Value::of_type<T>(obj));
		return (T *)obj;
	}
	
	class ValueMapFunctions:
		public MapFunctions<value_t, value_t>
	{
		public:
			static value_t invalid_value()
			{
				return value_undef;
			}
	};

	typedef Map<value_t, value_t, GC, ValueMapFunctions> ValueMap;
};

