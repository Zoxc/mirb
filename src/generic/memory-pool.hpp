#pragma once
#include "../common.hpp"
#include "allocator.hpp"
#include "simpler-list.hpp"

namespace Mirb
{
	class MemoryPool:
			public Allocator
	{
		struct Page
		{
			SimplerEntry<Page> entry;
			#ifndef WIN32
				size_t length;
			#endif
		};

		static const unsigned int max_alloc = 0x1000 * 4;

		private:
			char_t *current;
			char_t *max;

			SimplerList<Page> pages;

			char_t *allocate_page(size_t bytes = max_alloc);
			void free_page(Page *page);
			void *get_page(size_t bytes);
		public:
			typedef MemoryPool &Ref;
			typedef Allocator::Wrap<MemoryPool &> Storage;

			MemoryPool();
			~MemoryPool();
			
			void *alloc(size_t bytes);
			void *realloc(void *mem, size_t old_size, size_t new_size);
			void free(void *mem)
			{
			}
	};
};

void *operator new(size_t bytes, Mirb::MemoryPool &memory_pool) throw();
void operator delete(void *, Mirb::MemoryPool &memory_pool) throw();
void *operator new[](size_t bytes, Mirb::MemoryPool &memory_pool) throw();
void operator delete[](void *, Mirb::MemoryPool &memory_pool) throw();
