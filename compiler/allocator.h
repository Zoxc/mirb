#pragma once
#include "../globals.h"

struct allocator {
	unsigned char *page;
	unsigned char *next;
	kvec_t(void *) pages;
};

void allocator_init(struct allocator *allocator);
void allocator_free(struct allocator *allocator);
void *allocator_page_alloc(struct allocator *allocator, size_t length);

static inline void *allocator_alloc(struct allocator *allocator, size_t length)
{
	unsigned char* result = allocator->next;

	unsigned char* next = result + length;

	if(next >= allocator->page)
		return allocator_page_alloc(allocator, length);

	allocator->next = next;

	return result;
}

