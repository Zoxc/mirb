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
	
	class Tuple:
		public Value::Header
	{
		private:
		public:
			Tuple(size_t entries) : Value::Header(Value::InternalTuple), entries(entries) {}

			value_t &operator[](size_t index)
			{
				mirb_debug_assert(index < entries);

				return ((value_t *)((size_t)this + sizeof(Tuple)))[index];
			}
			
			size_t entries;

			template<typename F> void mark(F mark)
			{
				for(size_t i = 0; i < entries; ++i)
					mark((*this)[i]);
			}
	};
	
	typedef Prelude::Allocator::Standard AllocatorBase;
	
	class AllocatorPrivate
	{
		static Tuple *allocate_tuple(size_t size);

		template<class A, class BaseAllocator> friend class Allocator;
	};


	template<class A, class BaseAllocator> class Allocator:
			public BaseAllocator::ReferenceBase
	{
		public:
			typedef BaseAllocator Base;
			
			typename Base::Reference reference()
			{
				return Base::default_reference;
			}
			
			class Storage
			{
				private:
					Storage(Tuple *array) : array(array) {}
					
					friend class Allocator;
				public:
					Tuple *array;

					static const bool null_references = true;
					
					Storage()
					{
						#ifdef DEBUG
							array = nullptr;
						#endif
					}
					
					Storage &operator =(decltype(nullptr) null)
					{
						mirb_debug_assert(null == nullptr);
						
						array = nullptr;

						return *this;
					}
					
					Storage &operator =(const Storage &other)
					{
						array = other.array;

						return *this;
					}
					
					operator bool() const
					{
						return array != 0;
					}
					
					A &operator [](size_t index) const
					{
						mirb_debug_assert(array);
						
						return *(A *)(&(*array)[index]);
					}
			};
			
			Allocator(typename Base::Reference allocator = Base::default_reference) {}
			Allocator(const Allocator &array_allocator) {}
				
			Storage allocate(size_t size)
			{
				return Storage(AllocatorPrivate::allocate_tuple(size));
			}
			
			Storage reallocate(const Storage &old, size_t old_size, size_t new_size)
			{
				Tuple &result = *AllocatorPrivate::allocate_tuple(new_size);

				for(size_t i = 0; i < old_size; ++i)
					result[i] = (*old.array)[i];

				return Storage(&result);
			}

			void free(const Storage &array)	{}
	};
};
