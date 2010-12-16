#include "memory-pool.hpp"

#ifdef VALGRIND
	#include <cstring>
#endif

namespace Mirb
{
	MemoryPool::MemoryPool()
		: current(0),
		max(0)
	{
	}
	
	MemoryPool::~MemoryPool()
	{
		#ifdef VALGRIND
			for(std::vector<char_t *>::iterator page = pages.begin(); page != pages.end(); ++page)
				std::free(*page);
		#else
			auto page = pages.begin();

			while(page != pages.end())
			{
				Page *current = *page;
				++page;
				free_page(current);
			};
		#endif
	}
	
	char_t *MemoryPool::allocate_page(size_t bytes)
	{
		char_t *result;
		
		#ifdef WIN32
			result = (char_t *)VirtualAlloc(0, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		#else	
			result = (char_t *)mmap(0, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		#endif
		
		assert(result);

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
		if(mirb_unlikely(bytes > max_alloc))
			return allocate_page(bytes + sizeof(Page));

		char_t *result = allocate_page();

		max = result + (max_alloc - sizeof(Page));

		result = (char_t *)align((size_t)result, memory_align);

		current = result + bytes;
		
		return result;
	}
	
	void *MemoryPool::realloc(void *mem, size_t old_size, size_t new_size)
	{
		void *result = alloc(new_size);
			
		memcpy(result, mem, old_size);
			
		return result;
	}
	
	void *MemoryPool::alloc(size_t bytes)
	{
		char_t *result;

		#ifdef VALGRIND
			result = (char_t *)malloc(bytes);
			
			assert(result);

			pages.push_back(result);
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
	return memory_pool.alloc(bytes);
}

void operator delete(void *, Mirb::MemoryPool &memory_pool) throw()
{
}

void *operator new[](size_t bytes, Mirb::MemoryPool &memory_pool) throw()
{
	return memory_pool.alloc(bytes);
}

void operator delete[](void *, Mirb::MemoryPool &memory_pool) throw()
{
}
