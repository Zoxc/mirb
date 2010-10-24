#pragma once
#include <assert.h>

namespace Mirb
{
	template<class T> class SimplerEntry
	{
	public:
		SimplerEntry() : next(0) {}

		T *next;
	};

	template<class T, class E = T, SimplerEntry<E> E::*field = &E::entry> class SimplerList
	{
	public:
		SimplerList() : first(0) {}
		
		T *first;
		
		bool empty()
		{
			return first == 0;
		}
		
		void append(T *node)
		{
			assert(node);
			
			(node->*field).next = first;
			first = node;
		}
		
		class Iterator
		{
		private:
			T *current;

		public:
			Iterator(SimplerList &list) : current(list.first) {}

			void step()
			{
				current = static_cast<T *>((current->*field).next);
			}

			operator bool()
			{
				return current != 0;
			}

			T &operator ++()
			{
				step();
				return *current;
			}

			T &operator ++(int)
			{
				T *result = current;
				step();
				return *result;
			}

			T *operator*()
			{
				return current;
			}

			T &operator ()()
			{
				return *current;
			}
		};

		Iterator begin()
		{
			return Iterator(*this);
		}
	};
};
