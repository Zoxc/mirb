#include "gc.hpp"

#ifdef DEBUG
	#define GC_DEBUG
#endif

#define GC_THREADS

extern "C"
{
    #include <gc/include/gc.h>
};

namespace Mirb
{
	GC gc;

	void *GC::alloc(size_t bytes)
	{
		#ifdef NO_GC
			return std::malloc(bytes);
		#else
			return GC_MALLOC(bytes);
		#endif
	}

	void *GC::realloc(void *mem, size_t old, size_t bytes)
	{
		#ifdef NO_GC
			return std::realloc(mem, bytes);
		#else
			return GC_REALLOC(mem, bytes);
		#endif
	}

};

void *operator new(size_t bytes, Mirb::GC &gc) throw()
{
	return gc.alloc(bytes);
}

void operator delete(void *, Mirb::GC &gc) throw()
{
}

void *operator new[](size_t bytes, Mirb::GC &gc) throw()
{
	return gc.alloc(bytes);
}

void operator delete[](void *, Mirb::GC &gc) throw()
{
}
