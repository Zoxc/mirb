#pragma once
#include "../common.hpp"

namespace Mirb
{
	typedef size_t *bit_set_t;

	template<class A> class BitSetWrapper
	{
		protected:
			static const size_t unit_size = sizeof(size_t) * 8;
			
			size_t units;
			size_t bytes;
			typename A::Ref alloc_ref;

		public:
			BitSetWrapper(typename A::Ref alloc_ref, size_t size) : alloc_ref(alloc_ref)
			{
				size_t aligned = align(size, unit_size);

				units = aligned / unit_size;
				bytes = aligned / 8;
			}

			bit_set_t create()
			{
				bit_set_t set = (bit_set_t)alloc_ref.alloc(bytes);
				
				assert(set);
				
				return set;
			}
			
			bit_set_t create_clean()
			{
				bit_set_t set = create();

				for(size_t i = 0; i < units; ++i)
					set[i] = 0;

				return set;
			}

			typedef size_t (operation_t)(size_t, size_t);
			
			static size_t set_union(size_t lhs, size_t rhs)
			{
				return lhs | rhs;
			}

			static size_t set_difference(size_t lhs, size_t rhs)
			{
				return lhs & ~rhs;
			}

			bool equal(bit_set_t a, bit_set_t b)
			{
				for(size_t i = 0; i < units; ++i)
					if(a[i] != b[i])
						return false;

				return true;
			}
			
			bit_set_t dup(bit_set_t set)
			{
				bit_set_t result = create();

				for(size_t i = 0; i < units; ++i)
					result[i] = set[i];

				return result;
			}
			
			bit_set_t set_union(bit_set_t a, bit_set_t b)
			{
				bit_set_t result = create();

				for(size_t i = 0; i < units; ++i)
					result[i] = a[i] | b[i];

				return result;
			}
			
			bit_set_t mod_set_union(bit_set_t a, bit_set_t b)
			{
				for(size_t i = 0; i < units; ++i)
					a[i] = a[i] | b[i];

				return a;
			}
			
			bit_set_t set_difference(bit_set_t a, bit_set_t b)
			{
				bit_set_t result = create();

				for(size_t i = 0; i < units; ++i)
					result[i] = a[i] & ~b[i];

				return result;
			}
			
			bit_set_t mod_set_difference(bit_set_t a, bit_set_t b)
			{
				for(size_t i = 0; i < units; ++i)
					a[i] = a[i] & ~b[i];

				return a;
			}
			
			static bool get(bit_set_t set, size_t index)
			{
				return (set[index / unit_size] & (1 << (index % unit_size))) != 0;
			}
			
			static void set(bit_set_t set, size_t index)
			{
				set[index / unit_size] |= 1 << (index % unit_size);
			}
			
			static void clear(bit_set_t set, size_t index)
			{
				set[index / unit_size] &= ~(1 << (index % unit_size));
			}
	};
};
