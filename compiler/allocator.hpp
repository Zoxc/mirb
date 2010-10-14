#pragma once
#include "../globals.hpp"
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
#define vec_compiler_init_args , struct allocator *allocator
#define vec_compiler_mov , vec->allocator
#define VEC_COMPILER(type, name) \
	VEC_INIT(type, name, struct allocator *allocator;, vec_compiler_init_args, vec->allocator = allocator;, vec_compiler_mov, vec_compiler_malloc, vec_compiler_realloc, vec_compiler_free)

#define hash_compiler_malloc(size) allocator_alloc(h->allocator, size)
#define hash_compiler_malloc_bootstrap(size) allocator_alloc(allocator, size)
#define hash_compiler_realloc(old, old_size, new_size) allocator_realloc(h->allocator, old, old_size, new_size)
#define hash_compiler_free(obj)

#define HASH_COMPILER(name, key_t, val_t)								\
	HASH_INIT(name, key_t, val_t, 1, hash_int_hash_func, hash_int_hash_equal, struct allocator *allocator;, struct allocator *allocator, h->allocator = allocator;, hash_compiler_malloc_bootstrap, hash_compiler_malloc, hash_compiler_realloc, hash_compiler_free)

#define HASH_COMPILER_STR(name, val_t)								\
	HASH_INIT(name, const char *, val_t, 1, hash_str_hash_func, hash_str_hash_equal, struct allocator *allocator;, struct allocator *allocator, h->allocator = allocator;, hash_compiler_malloc_bootstrap, hash_compiler_malloc, hash_compiler_realloc, hash_compiler_free)

void allocator_init(struct allocator *allocator);
void allocator_free(struct allocator *allocator);
size_t allocator_page_alloc(struct allocator *allocator, size_t length);

static inline void *allocator_alloc(struct allocator *allocator, size_t length)
{
	#if VALGRIND && !ALLOCATOR_DEBUG
		return malloc(length);
	#else
		size_t result = allocator->next;

		length = RT_ALIGN(length, ALLOCATOR_ALIGN);

		size_t next = result + length;

		if(next >= allocator->page)
			return (void *)allocator_page_alloc(allocator, length);

		allocator->next = next;

		return (void *)result;
	#endif
}

static inline void *allocator_realloc(struct allocator *allocator, void *old, size_t old_length, size_t new_length)
{
	#if VALGRIND && !ALLOCATOR_DEBUG
		return realloc(old, new_length);
	#else
		void *result = allocator_alloc(allocator, new_length);

		memcpy(result, old, old_length);

		return result;
	#endif
}

