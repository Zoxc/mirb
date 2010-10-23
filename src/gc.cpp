#include "gc.hpp"
#include "../../runtime/runtime.hpp"

Mirb::GC Mirb::gc;

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
