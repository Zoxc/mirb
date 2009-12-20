#pragma once
#include "../globals.h"
#define ALLOCATOR_ALIGN sizeof(size_t)

VEC_DEFAULT(size_t, allocator_pages)

struct allocator {
	size_t page;
	size_t next;
	vec_t(allocator_pages) pages;
};

#define vec_compiler_malloc(size) allocator_alloc(vec->allocator, size)
#define vec_compiler_realloc(old, old_size, new_size) allocator_realloc(vec->allocator, old, old_size, new_size)
#define vec_compiler_free(obj)
#define vec_compiler_fields struct allocator *allocator;
#define vec_compiler_init_args , struct allocator *allocator
#define vec_compiler_mov , vec->allocator
#define VEC_COMPILER(type, name) \
	VEC_INIT(type, name, vec_compiler_fields, vec_compiler_init_args, vec->allocator = allocator;, vec_compiler_mov, vec_compiler_malloc, vec_compiler_realloc, vec_compiler_free)

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

