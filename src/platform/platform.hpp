#include "../common.hpp"
#include "../char-array.hpp"

namespace Mirb
{
	namespace Platform
	{
		enum Color
		{
			Green,
			Bold,
			Red,
			Gray
		};

		void *allocate_region(size_t bytes);
		void free_region(void *region, size_t bytes);

		void initialize();
	};
};

#ifdef WIN32
	#include "winapi.hpp"
#else
	#include "posix.hpp"
#endif
