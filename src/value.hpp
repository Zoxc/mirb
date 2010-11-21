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

	typedef size_t value_t;
	
	const value_t value_nil = 0;
	const value_t value_true = 2;
	const value_t value_false = 4;
	const value_t value_undef = 6;
	const value_t value_highest = value_undef;

	struct Value
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

		union cast
		{
			value_t value;
			
			cast(size_t value) : value(value) {}

			cast(bool value)
			{
				this->value = value ? value_true : value_false;
			}
		
			cast(Mirb::Object * object) : value((value_t)object) {}
			cast(Mirb::Symbol * symbol) : value((value_t)symbol) {}

			operator value_t() { return value; }
			
			operator struct rt_common *() { return (struct rt_common *)value; }
			operator struct rt_proc *() { return (struct rt_proc *)value; }
			operator struct rt_string *() { return (struct rt_string *)value; }
			operator struct rt_array *() { return (struct rt_array *)value; }

			operator Mirb::Object *() { return (Mirb::Object *)value; }
			operator Mirb::Symbol *() { return (Mirb::Symbol *)value; }
		};
		
		Type type(value_t value);
	};

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

