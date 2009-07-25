#include "../globals.h"

static unsigned char *heap;
static unsigned char *next;
static unsigned char *end;

#define CODE_HEAP_SIZE (1024 * 1024 * 16)

void code_heap_create(void)
{
	heap = (unsigned char *)VirtualAlloc(0, CODE_HEAP_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	assert(heap); // Unable to allocate code heap!

	next = heap;
	end = heap + CODE_HEAP_SIZE;
}

void code_heap_destroy(void)
{
}

void* code_heap_alloc(unsigned int size)
{
	unsigned char* result = next;

	next = next + size;

	assert(next < end); // Heap is out of memory!

	return result;
}
