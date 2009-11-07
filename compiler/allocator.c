#include "allocator.h"

#define ALLOCATOR_PAGE_SIZE 0x1000

static void *get_page(void)
{
	void *result;

	#ifdef WINDOWS
		result = VirtualAlloc(0, ALLOCATOR_PAGE_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		RT_ASSERT(result);
	#else
		result = mmap(0, ALLOCATOR_PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

		RT_ASSERT(result != MAP_FAILED);
	#endif

	return result;
}

static void free_page(void *page)
{
    #ifdef WINDOWS
		VirtualFree(page, 0, MEM_RELEASE);
	#else
		munmap(page, ALLOCATOR_PAGE_SIZE);
	#endif
}

void allocator_init(struct allocator *allocator)
{
	allocator->next = (unsigned char *)get_page();
	allocator->page = allocator->next + ALLOCATOR_PAGE_SIZE;

	kv_init(allocator->pages);
}

void allocator_free(struct allocator *allocator)
{
	free_page(allocator->page - ALLOCATOR_PAGE_SIZE);

	for(size_t i = 0; i < kv_size(allocator->pages); i++)
		free_page(kv_A(allocator->pages, i) - ALLOCATOR_PAGE_SIZE);

	kv_destroy(allocator->pages);
}

void *allocator_page_alloc(struct allocator *allocator, size_t length)
{
	kv_push(void *, allocator->pages, allocator->page);

	unsigned char* result = (unsigned char *)get_page();

	allocator->page = result + ALLOCATOR_PAGE_SIZE;
	allocator->next = result + length;

	return result;
}
