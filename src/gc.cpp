#include "gc.hpp"
#include "../runtime/runtime.hpp"

namespace Mirb
{
	GC gc;

	void *GC::alloc(size_t bytes)
	{
		return (void *)rt_alloc(bytes);
	}

	void *GC::realloc(void *mem, size_t old, size_t bytes)
	{
		return (void *)rt_realloc((rt_value)mem, bytes);
	}

};

void *operator new(size_t bytes, Mirb::GC &gc) throw()
{
	return (void *)rt_alloc(bytes);
}

void operator delete(void *, Mirb::GC &gc) throw()
{
}

void *operator new[](size_t bytes, Mirb::GC &gc) throw()
{
	return (void *)rt_alloc(bytes);
}

void operator delete[](void *, Mirb::GC &gc) throw()
{
}
