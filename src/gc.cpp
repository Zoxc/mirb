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

	void *GC::allocate(size_t bytes)
	{
		#ifdef VALGRIND
			return std::malloc(bytes);
		#elif NO_GC
			return gc_heap.allocate(bytes);
		#else
			return GC_MALLOC(bytes);
		#endif
	}

	void *GC::reallocate(void *memory, size_t old, size_t bytes)
	{
		#ifdef VALGRIND
			return std::realloc(memory, bytes);
		#elif NO_GC
			return gc_heap.reallocate(memory, old, bytes);
		#else
			return GC_REALLOC(memory, bytes);
		#endif
	}
};
