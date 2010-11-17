#pragma once
#include "common.hpp"

namespace Mirb
{
	class SimpleSet
	{
		private:
			size_t data;

		public:
			SimpleSet() : data(0)
			{
			}

			template<size_t index> size_t bit()
			{
				static_assert(index < (sizeof(size_t) * 8), "Index too high");

				return 1 << index;
			}

			template<size_t index> bool get()
			{
				return (data & bit<index>()) != 0;
			}
			
			template<size_t index> void set(bool value = true)
			{
				if(value)
					data |= bit<index>();
				else
					data &= ~bit<index>();
			}
			
			template<size_t index> void clear()
			{
				set<index>(false);
			}
			
	};
};
