#include "memory-pool.hpp"

#ifdef VALGRIND
	#include <cstring>
#endif

namespace Mirb
{
	MemoryPool::MemoryPool()
	{
		#ifndef VALGRIND
			current = current_page = allocate_page();

			max = current + max_alloc;
		#endif
	}
	
	MemoryPool::~MemoryPool()
	{
		for(std::vector<char_t *>::iterator page = pages.begin(); page != pages.end(); page++)
			free_page(*page);
		
		#ifndef VALGRIND
			free_page(current_page);
		#endif
	}
	
	char_t *MemoryPool::allocate_page(size_t bytes)
	{
		char_t *result;
		
		#ifdef WIN32
			result = (char_t *)VirtualAlloc(0, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		#else	
			result = (char_t *)std::malloc(bytes);
		#endif
		
		assert(result);
		
		return result;
	}
	
	void MemoryPool::free_page(char_t *page)
	{
		#ifdef WIN32
			VirtualFree((void *)page, 0, MEM_RELEASE);
		#else
			std::free(page);
		#endif
	}
	
	void *MemoryPool::get_page(size_t bytes)
	{
		if(bytes > max_alloc)
		{
			char_t *result = allocate_page(bytes);

			pages.push_back(result);

			return result;
		}

		pages.push_back(current_page);
		
		char_t *result = allocate_page();

		current_page = result;
		current = result + bytes;
		max = result + max_alloc;
		
		return result;
	}
	
	void *MemoryPool::allocate(size_t bytes)
	{
		char_t *result;

		#ifdef VALGRIND
			result = (char_t *)malloc(bytes);
			
			assert(result);

			pages.push_back(result);
		#else
			result = current;

			char_t *next = result + bytes;
		
			if(next >= max)
				return get_page(bytes);

			current = next;
		#endif

		return (void *)result;
	}
};

void *operator new(size_t bytes, Mirb::MemoryPool *memory_pool) throw()
{
	return memory_pool->allocate(bytes);
}

void operator delete(void *, Mirb::MemoryPool *memory_pool) throw()
{
}

void *operator new[](size_t bytes, Mirb::MemoryPool *memory_pool) throw()
{
	return memory_pool->allocate(bytes);
}

void operator delete[](void *, Mirb::MemoryPool *memory_pool) throw()
{
}

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
