#include "allocator.hpp"
#include "collector.hpp"

namespace Mirb
{
	Collector &collector = *(Collector *)nullptr;

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

void *operator new(size_t bytes, Mirb::Collector &) throw()
{
	Mirb::value_t result = (Mirb::value_t)Mirb::Collector::allocate_simple(bytes);

#ifdef DEBUG
	result->block_size = bytes;
#endif

	return result;
}

void operator delete(void *, Mirb::Collector &) throw()
{
}
