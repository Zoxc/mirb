#pragma once
#include "../globals.h"

typedef struct {
	unsigned char *page;
	unsigned char *next;
	kvec_t(void *) pages;
} allocator_t;

void allocator_init(allocator_t *allocator);
void allocator_free(allocator_t *allocator);
void *allocator_page_alloc(allocator_t *allocator, size_t length);

static inline void *allocator_alloc(allocator_t *allocator, size_t length)
{
	unsigned char* result = allocator->next;

	unsigned char* next = result + length;

	if(next >= allocator->page)
		return allocator_page_alloc(allocator, length);

	allocator->next = next;

	return result;
}

