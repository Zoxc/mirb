#pragma once
#include "common.hpp"
#include <Prelude/FastList.hpp>
#include <Prelude/LinkedList.hpp>
#include <Prelude/Allocator.hpp>
#include "value.hpp"
#include "value-map.hpp"
#include "char-array.hpp"
#include "classes/object.hpp"
#include "allocator.hpp"

namespace Mirb
{
	class Collector
	{
		private:
			prelude_align(struct, Region, mirb_object_align)
			{
				ListEntry<Region> entry;
				char_t *pos;
				char_t *end;

				value_t data()
				{
					return (value_t)((size_t)this + sizeof(Region));
				}
			};
			
			static bool pending;
			
			static void sweep();
			static void compact();
			static void mark();
			
			static void update_forward();
			static void update_backward();

			friend struct MarkRootFunc;
			friend struct ThreadFunc;

			static FastList<Region> regions;
			static const size_t page_size = 0x1000;
			
			friend struct RegionWalker;
			template<bool backward> friend struct RegionAllocator;
			
			static Region *current;
			static size_t pages;

			#ifdef VALGRIND
				static LinkedList<Value::Header> heap_list;
			#else
				static LinkedList<PinnedHeader, PinnedHeader, &PinnedHeader::entry> heap_list;
			#endif

			void test_sizes();
						
			static Region *allocate_region(size_t bytes);
			static void free_region(Region *region);
			static void *get_region(size_t bytes);

			static void *allocate_simple(size_t bytes)
			{
				mirb_debug_assert((bytes & object_ref_mask) == 0);
				
				memory += bytes;

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
				
				#ifdef DEBUG		
					object->size = sizeof(T);
				#endif

				#ifdef VALGRIND
					heap_list.append(object);
				#endif

				return object;
			}
			
			template<class T> static void *allocate_pinned_object()
			{
				return std::malloc(sizeof(T));
			}
			
			template<class T> static T *setup_pinned_object(T *object)
			{
				static_assert(std::is_base_of<PinnedHeader, T>::value, "T must be a PinnedHeader");
				
				#ifdef DEBUG		
					object->size = sizeof(T);
				#endif

				heap_list.append(object);

				return object;
			}
		public:
			static size_t collections;
			static size_t region_count;
			static unsigned long long memory;

			static void collect();
			static void initialize();
			static void free();

			static void check()
			{
				#ifdef DEBUG
					collect();
				#else
					if(prelude_unlikely(pending))
					{
						pending = false;
						collect();
					}
				#endif
			}
			
			static void *allocate(size_t bytes);
			static void *reallocate(void *memory, size_t old_size, size_t bytes);

			template<class T> static Tuple<T> &allocate_tuple(size_t entries)
			{
				size_t size = sizeof(Tuple<T>) + entries * sizeof(T *);

				auto tuple = new (allocate_simple(size)) Tuple<T>(entries);
				
				#ifdef DEBUG		
					tuple->size = size;
				#endif

				#ifdef VALGRIND
					heap_list.append(tuple);
				#endif

				return *tuple;
			}

			template<class T> static T *allocate_pinned()
			{
				return setup_pinned_object<T>(new (allocate_pinned_object<T>()) T());
			}
			
			template<class T, typename Arg1> static T *allocate_pinned(Arg1&& arg1)
			{
				return setup_pinned_object<T>(new (allocate_pinned_object<T>()) T(std::forward<Arg1>(arg1)));
			}
			
			template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4> static T *allocate_pinned(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4)
			{
				return setup_pinned_object<T>(new (allocate_pinned_object<T>()) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4)));
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
