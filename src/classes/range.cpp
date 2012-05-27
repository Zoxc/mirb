#include "range.hpp"
#include "fixnum.hpp"
#include "string.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Range::to_s(Range *obj)
	{
		OnStack<1> os1(obj);

		auto low = inspect(obj->low);

		OnStackString<1> os2(low);
		
		auto high = inspect(obj->high);

		return (low + (const char_t *)(obj->flag ? "..." : "..") + high).to_string();
	}
	
	void Range::convert_to_index(size_t &start, size_t &length, size_t size)
	{
		if(!Value::is_fixnum(low) || !Value::is_fixnum(high))
			raise(context->runtime_error, "Expected Fixnum range attributes");
		
		intptr_t left = Fixnum::to_int(low);
		intptr_t right = Fixnum::to_int(high);
		
		if(left < 0)
		{
			left = -left;

			if((size_t)left > size)
			{
				start = 0;
				length = 0;

				return;
			}
			else
				left = size - (size_t)left;
		}

		if(right < 0)
		{
			right = -right;

			if((size_t)right > size)
			{
				start = 0;
				length = 0;

				return;
			}
			else
				right = size - (size_t)right;
		}

		if((size_t)left >= size || left > right)
		{
			start = 0;
			length = 0;

			return;
		}
		else
		{
			start = left;
			length = right - left + (flag ? 0 : 1);

			if(start + length > size)
				length = size - start;

			return;
		}
	}

	Range *Range::allocate(value_t low, value_t high, bool exclusive)
	{
		Range *result = Collector::allocate<Range>(low, high, exclusive);

		return result;
	}
	
	void Range::initialize()
	{
		context->range_class = define_class("Range", context->object_class);
		
		method<Arg::Self<Arg::Class<Range>>, &to_s>(context->range_class, "to_s");
	}
};

