#include "allocator.h"

#define ALLOCATOR_PAGE_SIZE 0x1000

static size_t get_page(void)
{
	void *result;

	#if VALGRIND && ALLOCATOR_DEBUG
		result = malloc(ALLOCATOR_PAGE_SIZE);
	#else
		#ifdef WIN32
			result = VirtualAlloc(0, ALLOCATOR_PAGE_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

			RT_ASSERT(result);
		#else
			result = mmap(0, ALLOCATOR_PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

			RT_ASSERT(result != MAP_FAILED);
		#endif
	#endif

	return (size_t)result;
}

static void free_page(size_t page)
{
	#ifdef WIN32
		VirtualFree((void *)page, 0, MEM_RELEASE);
	#else
		munmap((void *)page, ALLOCATOR_PAGE_SIZE);
	#endif
}

void allocator_init(struct allocator *allocator)
{
	allocator->next = get_page();
	allocator->page = allocator->next + ALLOCATOR_PAGE_SIZE;

	vec_init(allocator_pages, &allocator->pages);
}

void allocator_free(struct allocator *allocator)
{
	free_page(allocator->page - ALLOCATOR_PAGE_SIZE);

	for(size_t i = 0; i < allocator->pages.size; i++)
		free_page(allocator->pages.array[i] - ALLOCATOR_PAGE_SIZE);

	vec_destroy(allocator_pages, &allocator->pages);
}

size_t allocator_page_alloc(struct allocator *allocator, size_t length)
{
	vec_push(allocator_pages, &allocator->pages, allocator->page);

	size_t result = get_page();

	allocator->page = result + ALLOCATOR_PAGE_SIZE;
	allocator->next = result + length, ALLOCATOR_ALIGN;

	return result;
}
