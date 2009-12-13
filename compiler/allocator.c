#include "allocator.h"

#define ALLOCATOR_PAGE_SIZE 0x1000

static size_t get_page(void)
{
	void *result;

	#ifdef WIN32
		result = VirtualAlloc(0, ALLOCATOR_PAGE_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		RT_ASSERT(result);
	#else
		result = mmap(0, ALLOCATOR_PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

		RT_ASSERT(result != MAP_FAILED);
	#endif

	return (size_t)result;
}

static void free_page(size_t page)
{
	#ifdef WIN32
		VirtualFree((void *)page, 0, MEM_RELEASE);
	#else
		munmap(page, ALLOCATOR_PAGE_SIZE);
	#endif
}

void allocator_init(struct allocator *allocator)
{
	allocator->next = get_page();
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

size_t allocator_page_alloc(struct allocator *allocator, size_t length)
{
	kv_push(size_t, allocator->pages, allocator->page);

	size_t result = get_page();

	allocator->page = result + ALLOCATOR_PAGE_SIZE;
	allocator->next = RT_ALIGN(result + length, ALLOCATOR_ALIGN);

	return result;
}
