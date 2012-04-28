#pragma once
#include "value.hpp"
#include "allocator.hpp"
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
		public MapFunctions<value_t, value_t>
	{
		public:
			typedef ValueMapPair Pair;

			static value_t invalid_value()
			{
				return value_raise;
			}

			static Pair *allocate_pair();

			template<typename Allocator> static Pair *allocate_pair(typename Allocator::Reference ref)
			{
				return allocate_pair();
			}

			template<typename Allocator> static void free_pair(typename Allocator::Reference ref, Pair *pair)
			{
			}
	};

	class ValueMap:
		public Value::Header
	{
		private:
			static const size_t vars_initial = 2;
			
		public:
			ValueMap() : Value::Header(Value::InternalValueMap), map(vars_initial) {}

			Map<value_t, value_t, ValueMapFunctions, Prelude::Allocator::Standard, Allocator> map;
			
			template<typename F> void mark(F mark)
			{
				map.mark(mark);
			}
	};

};

