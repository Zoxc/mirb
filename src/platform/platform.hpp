#include "../common.hpp"

namespace Mirb
{
	namespace Platform
	{
		void *allocate_region(size_t bytes);
		void free_region(void *region, size_t bytes);

		void initialize();
	};
};
