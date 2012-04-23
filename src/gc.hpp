#pragma once
#include "common.hpp"
#include <Prelude/Allocator.hpp>

namespace Mirb
{
	class GC:
		public NoReferenceProvider<GC>
	{
		public:			
			GC() {}
			GC(Ref::Type reference) {}
			GC(const GC &allocator) {}

			void *allocate(size_t bytes);
			void *reallocate(void *memory, size_t old, size_t bytes);

			static const bool can_free = false;

			void free(void *memory)
			{
			}
	};
	
	extern GC gc;
};
