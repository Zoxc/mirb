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

	typedef size_t value_t;
	
	const value_t value_nil = 0;
	const value_t value_true = 2;
	const value_t value_false = 4;
	const value_t value_undef = 6;
	const value_t value_highest = value_undef;

	namespace Value
	{
		enum Type {
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
			Exception,
			Types
		};

		bool test(value_t value);

		bool by_value(value_t value);
		
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
			debug_assert(Value::of_type<Object>(value));
		}

		auto_cast(Module *obj) : value((value_t)obj)
		{
			debug_assert(Value::of_type<Module>(value));
		}

		auto_cast(Class *obj) : value((value_t)obj)
		{
			debug_assert(Value::of_type<Class>(value));
		}
		
		auto_cast(Symbol *obj) : value((value_t)obj)
		{
			debug_assert(Value::of_type<Symbol>(value));
		}
		
		auto_cast(String *obj) : value((value_t)obj)
		{
			debug_assert(Value::of_type<String>(value));
		}

		auto_cast(Array *obj) : value((value_t)obj)
		{
			debug_assert(Value::of_type<Array>(value));
		}
		
		auto_cast(Exception *obj) : value((value_t)obj)
		{
			debug_assert(Value::of_type<Exception>(value));
		}

		auto_cast(Proc *obj) : value((value_t)obj)
		{
			debug_assert(Value::of_type<Proc>(value));
		}

		operator value_t() { return value; }
		
		operator Object *() { return (Object *)value; }
		operator Module *() { return (Module *)value; }
		operator Class *() { return (Class *)value; }
		operator Symbol *() { return (Symbol *)value; }
		operator String *() { return (String *)value; }
		operator Array *() { return (Array *)value; }
		operator Exception *() { return (Exception *)value; }
		operator Proc *() { return (Proc *)value; }
	};

	template<typename T> T *cast(value_t obj)
	{
		return auto_cast(obj);
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

