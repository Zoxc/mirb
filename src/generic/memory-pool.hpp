#pragma once
#include "../common.hpp"
#include <Prelude/Allocator.hpp>
#include <Prelude/FastList.hpp>

namespace Mirb
{
	class MemoryPool:
		public WithReferenceProvider<MemoryPool>
	{
		struct Page
		{
			ListEntry<Page> entry;
			#ifndef WIN32
				size_t length;
			#endif
		};

		static const unsigned int max_alloc = 0x1000 * 4;

		private:
			char_t *current;
			char_t *max;

			FastList<Page> pages;

			char_t *allocate_page(size_t bytes = max_alloc);
			void free_page(Page *page);
			void *get_page(size_t bytes);
		public:
			MemoryPool();
			~MemoryPool();
			
			void *allocate(size_t bytes);
			void *reallocate(void *memory, size_t old_size, size_t new_size);
			void free(void *memory)
			{
			}
	};
};

void *operator new(size_t bytes, Mirb::MemoryPool &memory_pool) throw();
void operator delete(void *, Mirb::MemoryPool &memory_pool) throw();
void *operator new[](size_t bytes, Mirb::MemoryPool &memory_pool) throw();
void operator delete[](void *, Mirb::MemoryPool &memory_pool) throw();
