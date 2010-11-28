#pragma once
#include "../common.hpp"

namespace Mirb
{
	namespace ExecutableHeap
	{
		void initialize();
		void finalize();
		
		void *alloc(size_t size);
		void *resize(void *data, size_t new_size);
	};
};
