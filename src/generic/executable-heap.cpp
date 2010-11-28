#include "executable-heap.hpp"

namespace Mirb
{
	namespace ExecutableHeap
	{
		void initialize();
		void finalize();
		
		void *alloc(size_t size);
		void *resize(void *data, size_t new_size);

		static unsigned char *heap;
		static unsigned char *next;
		static unsigned char *end;

		static const size_t code_heap_size = 1024 * 1024 * 16;
		static const size_t code_heap_align = 16;

		void initialize()
		{
			#ifdef WIN32
				heap = (unsigned char *)VirtualAlloc(0, code_heap_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

				runtime_assert(heap);
			#else
				heap = (unsigned char *)mmap(0, code_heap_size, PROT_READ | PROT_EXEC | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

				runtime_assert(heap != MAP_FAILED);
			#endif

			next = heap;
			end = heap + code_heap_size;
		}

		void finalize()
		{
		}

		void *alloc(size_t size)
		{
			unsigned char *result = next;

			next = (unsigned char *)align((size_t)next + size, code_heap_align);

			runtime_assert(next <= end); // Heap is out of memory!

			return result;
		}
		
		void *resize(void *data, size_t new_size)
		{
			return data;
		}
	};
};
