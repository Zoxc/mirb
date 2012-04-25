#pragma once
#include "common.hpp"
#include <Prelude/Allocator.hpp>

namespace Mirb
{
	class Allocator:
		public NoReferenceProvider<Allocator>
	{
		public:
			Allocator() {}
			Allocator(Ref::Type reference) {}
			Allocator(const Allocator &allocator) {}

			static void *allocate(size_t bytes);
			static void *reallocate(void *memory, size_t old_size, size_t bytes);
			static void free(void *memory) {}

			static const bool can_free = false;
	};
};
