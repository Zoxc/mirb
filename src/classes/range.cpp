#include "range.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Range::to_s(Range *obj)
	{
		OnStack<1> os(obj);

		CharArray low = inspect_obj(obj->low);

		OnStackString<1> oss(low);
		
		CharArray high = inspect_obj(obj->high);

		return (low + (const char_t *)(obj->flag ? "..." : "..") + high).to_string();
	}
	
	Range *Range::allocate(value_t low, value_t high, bool exclusive)
	{
		Range *result = Collector::allocate<Range>(low, high, exclusive);

		return result;
	}
	
	void Range::initialize()
	{
		context->range_class = define_class("Range", context->object_class);
		
		method<Arg::SelfClass<Range>>(context->range_class, "to_s", &to_s);
	}
};

