#pragma once
#include "../common.hpp"
#include "simple-list.hpp"

namespace Mirb
{
	template<class T, class E = T, SimpleEntry<E> E::*field = &E::entry> class CountedSimpleList:
		public SimpleList<T, E, field>
	{
	public:
		CountedSimpleList() : size(0) {}
		
		size_t size;
		
		void append(T *node)
		{
			size++;

			SimpleList<T, E, field>::append(node);
		}
	};
};
