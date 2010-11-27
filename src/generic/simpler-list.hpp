#pragma once
#include "../common.hpp"

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
			debug_assert(node);
			
			(node->*field).next = first;
			first = node;
		}
		
		class Iterator
		{
		private:
			T *current;

		public:
			Iterator(T *start) : current(start) {}

			void step()
			{
				current = static_cast<T *>((current->*field).next);
			}
			
			bool operator ==(const Iterator &other) const 
			{
				return current == other.current;
			}
				
			bool operator !=(const Iterator &other) const 
			{
				return current != other.current;
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
			
			T *operator*() const
			{
				return current;
			}

			T &operator ()() const
			{
				return *current;
			}
		};

		Iterator begin()
		{
			return Iterator(first);
		}
		
		Iterator end()
		{
			return Iterator(0);
		}
	};
};
