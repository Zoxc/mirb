#pragma once
#include <Prelude/Allocator.hpp>
#include "common.hpp"

namespace Mirb
{
	class GC:
		public Prelude::NoReferenceProvider<GC>
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
