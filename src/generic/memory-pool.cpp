#include "memory-pool.hpp"
#include "../platform/platform.hpp"

#ifdef VALGRIND
	#include <cstring>
#endif

namespace Mirb
{
	MemoryPoolImplementation::~MemoryPoolImplementation()
	{
		auto page = pages.begin();

		while(page != pages.end())
		{
			Page *current = *page;
			++page;
			free_page(current);
		};
	}
	
	MemoryPoolImplementation::MemoryPoolImplementation()
		: current(0),
		max(0)
	{
	}
		
	char_t *MemoryPoolImplementation::allocate_page(size_t bytes)
	{
		char_t *result = (char_t *)Platform::allocate_region(bytes);

		Page *page = new ((void *)result) Page;

		page->length = bytes;

		pages.append(page);

		return result + sizeof(Page);
	}
	
	void MemoryPoolImplementation::free_page(Page *page)
	{
		Platform::free_region((void *)page, page->length);
	}
	
	void *MemoryPoolImplementation::get_page(size_t bytes)
	{
		if(prelude_unlikely(bytes > (max_alloc - sizeof(Page))))
			return allocate_page(bytes + sizeof(Page));

		char_t *result = allocate_page();

		max = result + (max_alloc - sizeof(Page));

		result = (char_t *)align((size_t)result, memory_align);

		current = result + bytes;
		
		return result;
	}
	
	void *MemoryPoolImplementation::reallocate(void *memory, size_t old_size, size_t new_size)
	{
		void *result = allocate(new_size);
			
		memcpy(result, memory, old_size);
			
		return result;
	}
	
	void *MemoryPoolImplementation::allocate(size_t bytes)
	{
		char_t *result;

		#ifdef VALGRIND
			result = (char_t *)malloc(bytes);
			
			mirb_runtime_assert(result != 0);
		#else
			result = (char_t *)align((size_t)current, memory_align);

			char_t *next = result + bytes;
		
			if(prelude_unlikely(next > max))
				return get_page(bytes);

			current = next;
		#endif

		return (void *)result;
	}
};

void *operator new(size_t bytes, Mirb::MemoryPool memory_pool) throw()
{
	return memory_pool.allocate(bytes);
}

void operator delete(void *, Mirb::MemoryPool) throw()
{
}
