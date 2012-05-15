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
		public:
			Range(value_t low, value_t high, bool exclusive) : Object(Value::Range, context->range_class), low(low), high(high)
			{
				flag = exclusive;
			}

			bool convert_to_index(size_t &start, size_t &length, size_t size);
			
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
