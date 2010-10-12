#pragma once
#include <assert.h>

namespace Mirb
{
	template<class T> class SimpleEntry
	{
	public:
		SimpleEntry() : next(0) {}

		T *next;
	};

	template<class T, class E, SimpleEntry<E> E::*field> class SimpleList
	{
	public:
		SimpleList() : first(0), last(0) {}
		
		T *first;
		T *last;

		void append(T *node)
		{
			if(!node) 
				assert(0);

			SimpleEntry<T> &entry = node->*field;

			entry.next = 0;

			if(last)
			{
				SimpleEntry<T> &last_entry = last->*field;

				last_entry.next = static_cast<E *>(node);
				last = node;
			}
			else
			{
				first = node;
				last = node;
			}
		}

		class Iterator
		{
		private:
			T *current;

		public:
			Iterator(SimpleList &list) : current(list.first) {}

			void step()
			{
				SimpleEntry<T> &entry = current->*field;
				current = static_cast<T *>(entry.next);
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
