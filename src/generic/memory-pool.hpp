#pragma once
#include "../common.hpp"
#include <Prelude/Allocator.hpp>
#include <Prelude/FastList.hpp>

namespace Mirb
{
	class MemoryPoolImplementation
	{
		struct Page
		{
			ListEntry<Page> entry;
			size_t length;
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
			MemoryPoolImplementation();
			~MemoryPoolImplementation();
			
			static const bool can_free = false;
			static const bool null_references = false;
			
			void *allocate(size_t bytes);
			void *reallocate(void *memory, size_t old_size, size_t new_size);
			void free(void *) {}
	};
	
	typedef Prelude::Allocator::ReferenceTemplate<MemoryPoolImplementation> MemoryPool;
};

void *operator new(size_t bytes, Mirb::MemoryPool memory_pool) throw();
static inline void operator delete(void *bytes, Mirb::MemoryPool memory_pool) throw() {}
