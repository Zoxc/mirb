#pragma once
#include "value.hpp"
#include <Prelude/Map.hpp>

namespace Mirb
{
	class ValueMapPair:
		public Value::Header
	{
		public:
			ValueMapPair() : Value::Header(Value::InternalValueMapPair) {}

			value_t key;
			value_t value;
			ValueMapPair *next;

			template<typename F> void mark(F mark)
			{
				mark(key);
				mark(value);

				if(next)
					mark(next);
			}
	};

	class ValueMapFunctions:
		public MapFunctions<value_t, value_t, Allocator>
	{
		public:
			typedef ValueMapPair Pair;

			static value_t invalid_value()
			{
				return value_raise;
			}

			static Pair *allocate_pair();

			static Pair *allocate_pair(NoReferenceProvider<Allocator> *allocator)
			{
				return allocate_pair();
			}
	};

	class ValueMap:
		public Value::Header
	{
		private:
			static const size_t vars_initial = 2;
			
		public:
			ValueMap() : Value::Header(Value::InternalValueMap), map(vars_initial) {}

			Map<value_t, value_t, Allocator, ValueMapFunctions> map;
			
			template<typename F> void mark(F mark)
			{
				map.mark(mark);
			}
	};

};

