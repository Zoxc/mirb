#pragma once
#include "common.hpp"
#include "gc.hpp"
#include "map.hpp"
#include "../runtime/runtime.hpp"

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

		Value() {}

		Value(bool value)
		{
			raw = value ? value_true : value_false;
		}
		
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

