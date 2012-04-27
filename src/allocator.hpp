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
	
	class VariableBlock:
		public Value::Header
	{
		private:
		public:
			VariableBlock(size_t bytes) : Value::Header(Value::InternalVariableBlock), bytes(bytes) {}

			static VariableBlock &from_memory(const void *memory)
			{
				auto result = (VariableBlock *)((const char_t *)memory - sizeof(VariableBlock));

				Value::assert_valid_skip_mark(result);
				mirb_debug_assert(result->get_type() == Value::InternalVariableBlock);

				return *result;
			}

			void *data()
			{
				return (void *)((size_t)this + sizeof(VariableBlock));
			}
			
			size_t bytes;

			template<typename F> void mark(F mark)
			{
			}
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
