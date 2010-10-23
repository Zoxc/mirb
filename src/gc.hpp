#pragma once
#include "common.hpp"

namespace Mirb
{
	class GC
	{
	};
	
	extern GC gc;
};

void *operator new(size_t bytes, Mirb::GC &gc) throw();
void operator delete(void *, Mirb::GC &gc) throw();
void *operator new[](size_t bytes, Mirb::GC &gc) throw();
void operator delete[](void *, Mirb::GC &gc) throw();
