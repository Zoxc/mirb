#pragma once
#include "common.hpp"
#include "gc.hpp"
#include "map.hpp"
#include "../runtime/runtime.hpp"

struct rt_common;
struct rt_proc;
struct rt_string;
struct rt_array;

namespace Mirb
{
	class Object;
	class Symbol;
	class Class;

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

		bool by_value(value_t value);
		
		template<class T> bool of_type(value_t value)
		{
			return false;
		}
		
		template<> bool of_type<Mirb::Object>(value_t value);
		template<> bool of_type<Mirb::Class>(value_t value);
		template<> bool of_type<Mirb::Symbol>(value_t value);

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

		auto_cast(Class *obj) : value((value_t)obj)
		{
			debug_assert(Value::of_type<Class>(value));
		}

		auto_cast(Symbol *obj) : value((value_t)obj)
		{
			debug_assert(Value::of_type<Symbol>(value));
		}

		operator value_t() { return value; }
			
		operator struct rt_common *() { return (struct rt_common *)value; }
		operator struct rt_proc *() { return (struct rt_proc *)value; }
		operator struct rt_string *() { return (struct rt_string *)value; }
		operator struct rt_array *() { return (struct rt_array *)value; }
			
		operator Object *() { return (Object *)value; }
		operator Class *() { return (Class *)value; }
		operator Symbol *() { return (Symbol *)value; }
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

