#pragma once
#include "common.hpp"
#include <Prelude/FastList.hpp>
#include "value.hpp"

namespace Mirb
{
	// PinnedHeader are objects with a fixed memory address

	class PinnedHeader:
		public Value::Header
	{
		private:
			ListEntry<PinnedHeader> entry;

			friend class Collector;
		public:
			PinnedHeader(Value::Type type) : Value::Header(type) {}
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
	};
	
	class VariableBlock:
		public Value::Header
	{
		private:
		public:
			VariableBlock(size_t bytes) : Value::Header(Value::InternalVariableBlock), bytes(bytes) {}

			static VariableBlock &from_memory(void *memory)
			{
				return *(VariableBlock *)((char_t *)memory - sizeof(VariableBlock));
			}

			void *data()
			{
				return (void *)((size_t)this + sizeof(VariableBlock));
			}
			
			size_t bytes;
	};
	
	class Collector
	{
		private:
			prelude_align(mirb_object_align) struct Region
			{
				ListEntry<Region> entry;
				char_t *pos;
				char_t *end;
			};

			static FastList<Region> regions;
			static const size_t page_size = 0x1000;
			
			static Region *current;
			static size_t pages;
			static FastList<PinnedHeader> pinned_object_list;

			void test_sizes();

			static void thread(value_t *node);
			static void update(value_t node, value_t free);
			static void move(value_t p, value_t new_p);
			static void update_forward();
			static void update_backward();
			
			static Region *allocate_region(size_t bytes);
			static void free_region(Region *region);
			static void *get_region(size_t bytes);

			static void *allocate_simple(size_t bytes)
			{
				mirb_debug_assert((bytes & object_ref_mask) == 0);

				char_t *result;

				#ifdef VALGRIND
					result = (char_t *)std::malloc(bytes);
			
					mirb_runtime_assert(result != 0);
				#else
					result = current->pos;

					char_t *next = result + bytes;
		
					mirb_debug_assert(((size_t)next & object_ref_mask) == 0);

					if(prelude_unlikely(next > current->end))
						return get_region(bytes);

					current->pos = next;
				#endif

				mirb_debug_assert(((size_t)result & object_ref_mask) == 0);

				return (void *)result;
			}

			template<class T> static void *allocate_object()
			{
				return allocate_simple(sizeof(T));
			}
			
			template<class T> static T *setup_object(T *object)
			{
				static_assert(std::is_base_of<Value::Header, T>::value, "T must be a Value::Header");
				
				return object;
			}
			
			template<class T> static void *allocate_pinned_object()
			{
				return std::malloc(sizeof(T));
			}
			
			template<class T> static T *setup_pinned_object(T *object)
			{
				static_assert(std::is_base_of<PinnedHeader, T>::value, "T must be a PinnedHeader");

				return object;
			}
		public:
			static void collect();
			
			static void initialize();

			class Allocator:
				public NoReferenceProvider<Allocator>
			{
				public:
					Allocator() {}
					Allocator(Ref::Type reference) {}
					Allocator(const Allocator &allocator) {}

					static void *allocate(size_t bytes)
					{
						bytes += sizeof(VariableBlock);

						return (new (allocate_simple(align(bytes, mirb_object_align))) VariableBlock(bytes))->data();
					}

					static void *reallocate(void *memory, size_t old_size, size_t bytes)
					{
						if(memory)
							mirb_debug_assert(VariableBlock::from_memory(memory).bytes - sizeof(VariableBlock) <= old_size);

						bytes += sizeof(VariableBlock);

						void *result = (new (allocate_simple(align(bytes, mirb_object_align))) VariableBlock(bytes))->data();

						memcpy(result, memory, old_size);
			
						return result;
					}

					static const bool can_free = false;

					static void free(void *memory) {}
			};
			
			static Tuple &allocate_tuple(size_t entries)
			{
				void *result = allocate_simple(sizeof(Tuple) + entries * sizeof(value_t));

				return *new (result) Tuple(entries);
			}

			template<class T> static T *allocate_pinned()
			{
				return setup_pinned_object<T>(new (allocate_pinned_object<T>()) T());
			}
			
			template<class T, typename Arg1> static T *allocate_pinned(Arg1&& arg1)
			{
				return setup_pinned_object<T>(new (allocate_pinned_object<T>()) T(std::forward<Arg1>(arg1)));
			}
			
			template<class T> static T *allocate()
			{
				return setup_object<T>(new (allocate_object<T>()) T());
			}
			
			template<class T, typename Arg1> static T *allocate(Arg1&& arg1)
			{
				return setup_object<T>(new (allocate_object<T>()) T(std::forward<Arg1>(arg1)));
			}
			
			template<class T, typename Arg1, typename Arg2> static T *allocate(Arg1&& arg1, Arg2&& arg2)
			{
				return setup_object<T>(new (allocate_object<T>()) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2)));
			}
			
			template<class T,  typename Arg1, typename Arg2, typename Arg3> static T *allocate(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3)
			{
				return setup_object<T>(new (allocate_object<T>()) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3)));
			}
			
			template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4> static T *allocate(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4)
			{
				return setup_object<T>(new (allocate_object<T>()) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4)));
			}
			
			template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> static T *allocate(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg4&& arg5)
			{
				return setup_object<T>(new (allocate_object<T>()) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5)));
			}
			
			template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6> static T *allocate(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6)
			{
				return setup_object<T>(new (allocate_object<T>()) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6)));
			}
			
			template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7> static T *allocate(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6, Arg7&& arg7)
			{
				return setup_object<T>(new (allocate_object<T>()) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6), std::forward<Arg7>(arg7)));
			}
			
			template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8> static T *allocate(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6, Arg7&& arg7, Arg8&& arg8)
			{
				return setup_object<T>(new (allocate_object<T>()) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6), std::forward<Arg7>(arg7), std::forward<Arg8>(arg8)));
			}
	};
};
