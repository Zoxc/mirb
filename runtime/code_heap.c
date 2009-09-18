#include "../globals.h"

static unsigned char *heap;
static unsigned char *next;
static unsigned char *end;

#define CODE_HEAP_SIZE (1024 * 1024 * 16)

void rt_code_heap_create(void)
{
	#ifdef WINDOWS
		heap = (unsigned char *)VirtualAlloc(0, CODE_HEAP_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

		assert(heap);
	#else
		heap = mmap(0, CODE_HEAP_SIZE, PROT_READ | PROT_EXEC | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

		assert(heap != MAP_FAILED);
	#endif

	next = heap;
	end = heap + CODE_HEAP_SIZE;
}

void rt_code_heap_destroy(void)
{
}

void* rt_code_heap_alloc(size_t size)
{
	unsigned char* result = next;

	next = next + size;

	assert(next < end); // Heap is out of memory!

	return result;
}
