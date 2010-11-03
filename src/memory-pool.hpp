#pragma once
#include "common.hpp"
#include "allocator.hpp"

namespace Mirb
{
	class MemoryPool:
			public Allocator
	{
		static const unsigned int max_alloc = 0x1000 * 4;

		private:
			std::vector<char_t *> pages;

			char_t *current_page;
			char_t *current;
			char_t *max;

			char_t *allocate_page(size_t bytes = max_alloc);
			void free_page(char_t *page);

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
