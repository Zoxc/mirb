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

	template<class T, class E = T, SimpleEntry<E> E::*field = &E::entry> class SimpleList
	{
	public:
		SimpleList() : first(0), last(0) {}
		
		T *first;
		T *last;
		
		bool empty()
		{
			return first == 0;
		}

		void append(T *node)
		{
			assert(node);
			
			( node->*field).next = 0;
			
			if(last)
			{
				(last->*field).next = static_cast<E *>(node);
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

		class MutableIterator
		{
		private:
			SimpleList &list;
			T *current;
			T *prev;

		public:
			MutableIterator(SimpleList &list) : list(list), current(list.first), prev(0) {}

			void step()
			{
				prev = current;
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
			
			void replace(T *node)
			{
				if(prev)
				{
					prev->*field.next = static_cast<E *>(node); 
				}
				else
				{
					list.first = node;
				}
				
				node->*field.next = this->current->*field.next; 
				
				if(node->*field.next == 0)
					list.last = node;
				
				this->current = node;
			}

			void insert(T *node)
			{
				if(prev)
					(prev->*field).next = static_cast<E *>(node); 
				else
					list.first = node;
				
				if(this->current)
					(node->*field).next = (this->current->*field).next; 
				else
				{
					list.last = node;
					(node->*field).next = 0;
				}
			}
		};

		MutableIterator mutable_iterator()
		{
			return MutableIterator(*this);
		}
	};
};
