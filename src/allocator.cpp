#include "allocator.hpp"
#include "collector.hpp"

namespace Mirb
{
	const value_t AllocatorPrivate::Null<value_t>::value = value_undef;

	Tuple<Object> *TupleBase::allocate_tuple(size_t size)
	{
		return &Collector::allocate_tuple<Object>(size);
	}

	Tuple<Value::Header> *TupleBase::allocate_value_tuple(size_t size)
	{
		return &Collector::allocate_tuple<Value::Header>(size);
	}
};
