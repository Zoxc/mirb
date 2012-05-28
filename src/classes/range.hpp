#pragma once
#include "class.hpp"
#include "../context.hpp"

namespace Mirb
{
	class Range:
		public Object
	{
		private:
			static value_t to_s(Range *obj);
			static value_t first(Range *self);
			static value_t last(Range *self);
			static value_t exclude_end(Range *self);
			static value_t include(Range *self, value_t value);

		public:
			Range(value_t low, value_t high, bool exclusive) : Object(Value::Range, context->range_class), low(low), high(high)
			{
				flag = exclusive;
			}

			void convert_to_index(size_t &start, size_t &length, size_t size);
			
			value_t low;
			value_t high;
			
			template<typename F> void mark(F mark)
			{
				Object::mark(mark);
				
				mark(low);
				mark(high);
			}

			static Range *allocate(value_t low, value_t high, bool exclusive);

			static void initialize();
	};
};
