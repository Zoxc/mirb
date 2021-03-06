#include "range.hpp"
#include "fixnum.hpp"
#include "string.hpp"
#include "../runtime.hpp"
#include "../collector.hpp"

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
	
	value_t Range::first(Range *self)
	{
		return self->low;
	}
	
	value_t Range::last(Range *self)
	{
		return self->high;
	}
	
	value_t Range::exclude_end(Range *self)
	{
		return Value::from_bool(self->flag);
	}
	
	value_t Range::include(Range *self, value_t value)
	{
		OnStack<2> os(self, value);

		try
		{
			if(compare(self->low, value) > 0)
				return value_false;

			if(self->flag)
				return Value::from_bool(compare(self->high, value) > 0);
			else
				return Value::from_bool(compare(self->high, value) >= 0);
		} catch(const InternalException &e)
		{
			if(!e.value->kind_of(context->standard_error))
				throw;

			return value_false;
		}
	}
	
	value_t Range::equal(Range *self, value_t value)
	{
		Range *other = try_cast<Range>(value);

		if(!other)
			return false;

		OnStack<2> os(self, other);
		
		if(!call(self->low, "==", other->low)->test())
			return value_false;

		if(!call(self->high, "==", other->high)->test())
			return value_false;

		return Value::from_bool(self->flag == other->flag);
	}
	
	void Range::initialize()
	{
		context->range_class = define_class("Range", context->object_class);
		
		method<Self<Range>, Value, &include>(context->range_class, "include?");
		method<Self<Range>, Value, &include>(context->range_class, "===");
		method<Self<Range>, Value, &equal>(context->range_class, "==");
		method<Self<Range>, &exclude_end>(context->range_class, "exclude_end?");
		method<Self<Range>, &first>(context->range_class, "first");
		method<Self<Range>, &last>(context->range_class, "last");

		method<Self<Range>, &to_s>(context->range_class, "to_s");
	}
};

