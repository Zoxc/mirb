#pragma once
#include "common.hpp"
#include "allocator.hpp"

namespace Mirb
{
	class GC:
		public Allocator
	{
		public:
			typedef GC Ref;
			typedef GC Storage;
			
			GC() {}
			GC(bool dummy) {}

			static bool def_ref()
			{
				return true;
			}
			
			void *alloc(size_t bytes);
			void *realloc(void *mem, size_t old, size_t bytes);

			static const bool can_free = false;

			void free(void *mem)
			{
			}
	};
	
	extern GC gc;
};

void *operator new(size_t bytes, Mirb::GC &gc) throw();
void operator delete(void *, Mirb::GC &gc) throw();
void *operator new[](size_t bytes, Mirb::GC &gc) throw();
void operator delete[](void *, Mirb::GC &gc) throw();
