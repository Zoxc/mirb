#include "platform.hpp"

#ifdef WIN32

namespace Mirb
{
	namespace Platform
	{
		void *allocate_region(size_t bytes)
		{
			void *result = VirtualAlloc(0, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			mirb_runtime_assert(result != 0);

			return result;
		}
	
		void free_region(void *region, size_t bytes)
		{
			VirtualFree(region, 0, MEM_RELEASE);
		}

		void initialize()
		{
		}
	};
};

#endif