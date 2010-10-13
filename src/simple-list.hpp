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
		protected:
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

		class MutableIterator:
			public Iterator
		{
		private:
			SimpleList &list;
			T *prev;

		public:
			MutableIterator(SimpleList &list) : Iterator(list), list(list), prev(0) {}

			void step()
			{
				prev = this->current;
				Iterator::step();
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
				
				(node->*field).next = (this->current->*field).next; 
				
				if(this->current == 0)
					list.last = node;
			}
		};

		MutableIterator mutable_iterator()
		{
			return MutableIterator(*this);
		}
	};
};
