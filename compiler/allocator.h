#pragma once
#include "../globals.h"
#define ALLOCATOR_ALIGN sizeof(size_t)

struct allocator {
	size_t page;
	size_t next;
	kvec_t(size_t) pages;
};

void allocator_init(struct allocator *allocator);
void allocator_free(struct allocator *allocator);
size_t allocator_page_alloc(struct allocator *allocator, size_t length);

static inline void *allocator_alloc(struct allocator *allocator, size_t length)
{
	#ifdef VALGRIND
		return malloc(length);
	#else
		size_t result = allocator->next;

		size_t next = result + length;

		if(next >= allocator->page)
			return (void *)allocator_page_alloc(allocator, length);

		allocator->next = next;

		return (void *)result;
	#endif
}

static inline void *allocator_realloc(struct allocator *allocator, void *old, size_t old_length, size_t new_length)
{
	#ifdef VALGRIND
		return realloc(old, newlength);
	#else
		void *result = allocator_alloc(allocator, new_length);

		memcpy(result, old, old_length);

		return result;
	#endif
}

