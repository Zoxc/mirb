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
	const size_t value_nil = 0;
	const size_t value_true = 2;
	const size_t value_false = 4;
	const size_t value_undef = 6;
	const size_t value_highest = value_undef;
	
	class Object;
	class Symbol;
	
	union Value
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

		size_t raw;
		rt_value value;
		Mirb::Object *object;
		Mirb::Symbol *symbol;

		struct rt_common *rt_common;
		struct rt_proc *rt_proc;
		struct rt_string *rt_string;
		struct rt_array *rt_array;

		Value() {}

		Value(bool value)
		{
			raw = value ? value_true : value_false;
		}
		
		Value(size_t value) : raw(value) {}
		
		// TODO: Remove this
		operator rt_value() { return raw; }
		
		operator struct rt_common *() { return rt_common; }
		operator struct rt_proc *() { return rt_proc; }
		operator struct rt_string *() { return rt_string; }
		operator struct rt_array *() { return rt_array; }

		// Typecasting to native pointers

		Value(Mirb::Object * object) : object(object) {}
		Value(Mirb::Symbol * symbol) : symbol(symbol) {}

		operator Mirb::Object *() { return object; }
		operator Mirb::Symbol *() { return symbol; }
		
		bool test()
		{
			return (raw & ~(value_false | value_true)) != 0;
		}
		
		Type type();
	};
	
	class ValueMapFunctions:
		public MapFunctions<Value, Value>
	{
		public:
			static Value invalid_value()
			{
				return value_undef;
			}
	};

	typedef Map<Value, Value, GC, ValueMapFunctions> ValueMap;
};

