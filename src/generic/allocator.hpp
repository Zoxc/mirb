#pragma once
#include <cstdlib>

namespace Mirb
{
	class Allocator
	{
		public:
			template<class T> class Wrap
			{
				private:
					T ref;
				public:
					Wrap(T ref) : ref(ref) {}

					
					static Wrap &def_ref()
					{
						assert(0); // This should never be called!

						return *(Wrap *)0;
					}

					operator T()
					{
						return ref;
					}

					void *alloc(size_t bytes)
					{
						return ref.alloc(bytes);
					}

					void *realloc(void *mem, size_t old_size, size_t new_size)
					{
						return ref.realloc(mem, old_size, new_size);
					}

					void free(void *mem)
					{
						return ref.free(mem);
					}
			};
	};

	class StdLibAllocator:
		public Allocator
	{
		public:
			typedef StdLibAllocator Ref;
			typedef StdLibAllocator Storage;

			StdLibAllocator(bool dummy)
			{
			}
			
			static bool def_ref()
			{
				return true;
			}

			void *alloc(size_t bytes)
			{
				return std::malloc(bytes);
			}

			void *realloc(void *table, size_t old, size_t bytes)
			{
				return std::realloc(table, bytes);
			}

			static const bool can_free = true;

			void free(void *table)
			{
				return std::free(table);
			}
	};
	
};
