#pragma once
#include "common.hpp"
#include <Prelude/Allocator.hpp>
#include <Prelude/LinkedList.hpp>
#include "value.hpp"

namespace Mirb
{
	// PinnedHeader are objects with a fixed memory address

	class PinnedHeader:
		public Value::Header
	{
		private:
			#ifndef VALGRIND
				LinkedListEntry<PinnedHeader> entry;
			#endif

			friend class Collector;
		public:
			PinnedHeader(Value::Type type) : Value::Header(type) {}
	};

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
