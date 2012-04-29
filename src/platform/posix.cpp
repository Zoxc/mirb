#include "platform.hpp"

#ifndef WIN32

namespace Mirb
{
	namespace Platform
	{
		void *allocate_region(size_t bytes)
		{
			void *result = mmap(0, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

			mirb_runtime_assert(result != 0);

			return result;
		}
		
		void free_region(void *region, size_t bytes)
		{
			munmap(region, bytes);
		}

		void initialize()
		{
		}
	};
};

#endif