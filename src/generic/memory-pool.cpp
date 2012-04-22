#include "memory-pool.hpp"

#ifdef VALGRIND
	#include <cstring>
#endif

namespace Mirb
{
	MemoryPool::~MemoryPool()
	{
		auto page = pages.begin();

		while(page != pages.end())
		{
			Page *current = *page;
			++page;
			free_page(current);
		};
	}
	
	MemoryPool::MemoryPool()
		: current(0),
		max(0)
	{
	}
		
	char_t *MemoryPool::allocate_page(size_t bytes)
	{
		char_t *result;
		
		#ifdef WIN32
			result = (char_t *)VirtualAlloc(0, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		#else	
			result = (char_t *)mmap(0, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		#endif
		
		mirb_runtime_assert(result != 0);

		Page *page = new ((void *)result) Page;

		#ifndef WIN32
			page->length = bytes;
		#endif

		pages.append(page);

		return result + sizeof(Page);
	}
	
	void MemoryPool::free_page(Page *page)
	{
		#ifdef WIN32
			VirtualFree((void *)page, 0, MEM_RELEASE);
		#else
			munmap((void *)page, page->length);
		#endif
	}
	
	void *MemoryPool::get_page(size_t bytes)
	{
		if(mirb_unlikely(bytes > (max_alloc - sizeof(Page))))
			return allocate_page(bytes + sizeof(Page));

		char_t *result = allocate_page();

		max = result + (max_alloc - sizeof(Page));

		result = (char_t *)align((size_t)result, memory_align);

		current = result + bytes;
		
		return result;
	}
	
	void *MemoryPool::reallocate(void *memory, size_t old_size, size_t new_size)
	{
		void *result = allocate(new_size);
			
		memcpy(result, memory, old_size);
			
		return result;
	}
	
	void *MemoryPool::allocate(size_t bytes)
	{
		char_t *result;

		#ifdef VALGRIND
			result = (char_t *)malloc(bytes);
			
			mirb_runtime_assert(result != 0);
		#else
			result = (char_t *)align((size_t)current, memory_align);

			char_t *next = result + bytes;
		
			if(mirb_unlikely(next > max))
				return get_page(bytes);

			current = next;
		#endif

		return (void *)result;
	}
};

void *operator new(size_t bytes, Mirb::MemoryPool &memory_pool) throw()
{
	return memory_pool.allocate(bytes);
}

void operator delete(void *, Mirb::MemoryPool &memory_pool) throw()
{
}

void *operator new[](size_t bytes, Mirb::MemoryPool &memory_pool) throw()
{
	return memory_pool.allocate(bytes);
}

void operator delete[](void *, Mirb::MemoryPool &memory_pool) throw()
{
}
