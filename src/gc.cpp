#include "gc.hpp"

#ifndef NO_GC
	#ifdef DEBUG
		#define GC_DEBUG
	#endif

	#define GC_THREADS

	extern "C"
	{
		#include <gc/include/gc.h>
	};
#else
	#include "generic/memory-pool.hpp"
#endif

namespace Mirb
{
	#if defined(NO_GC) && !defined(VALGRIND)
		static MemoryPool gc_heap;
	#endif

	GC gc;

	void *GC::alloc(size_t bytes)
	{
		#ifdef VALGRIND
			return std::malloc(bytes);
		#elif NO_GC
			return gc_heap.alloc(bytes);
		#else
			return GC_MALLOC(bytes);
		#endif
	}

	void *GC::realloc(void *mem, size_t old, size_t bytes)
	{
		#ifdef VALGRIND
			return std::realloc(mem, bytes);
		#elif NO_GC
			return gc_heap.realloc(mem, old, bytes);
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
