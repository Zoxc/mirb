#pragma once
#include <cstring>

namespace Mirb
{
	template<class S> class VectorFunctions
	{
		public:
			static void *alloc(S storage, size_t bytes)
			{
				return malloc(bytes);
			}

			static void *realloc(S storage, void *table, size_t old, size_t bytes)
			{
				return realloc(table, bytes);
			}

			static void free(S storage, void *table)
			{
				return free(table);
			}
	};

	template<class T, class F> class Vector
	{
		private:
			T *table;
			typename F::Storage storage;
			size_t entries;
			size_t size;
		
		public:
			Vector(size_t initial, typename F::Storage storage) : storage(storage)
			{
				entries = 0;
				size = 1 << initial;
				
				table = (T *)F::alloc(storage, sizeof(T) * size);
			}
			
			Vector(typename F::Storage storage) : storage(storage)
			{
				entries = 0;
				size = 0;
				table = 0;
			}
			
			~Vector()
			{
				F::free(storage, (void *)table);
			}
			
			T *first()
			{
				return table;
			}
			
			T *last()
			{
				return &table[entries];
			}
			
			void push(T entry)
			{
				if(entries + 1 > size)
				{
					if(table)
					{
						size <<= 1;
						table = (T *)F::realloc(storage, (void *)table, sizeof(T) * (size >> 1), sizeof(T) * size);
					}
					else
					{
						size = 1;
						table = (T *)F::alloc(storage, sizeof(T) * size);
					}
				}
				
				table[entries++] = entry;
			}
			
			T *find(T entry)
			{
				for(auto i = begin(); i; i++)
				{
					if(i() == entry)
						return i.position();
				}
				
				return 0;
			}
			
			class Iterator
			{
			private:
				T *current;
				T *end;

			public:
				Iterator(Vector &vector, T *start, T *end) : current(start), end(end) {}
				
				operator bool()
				{
					return current != end;
				}
			
				T *position()
				{
					return current;
				}
				
				T &operator ++()
				{
					return *++current;
				}
				
				T &operator ++(int)
				{
					return *current++;
				}
				
				T *operator*()
				{
					return *current;
				}
				
				T &operator ()()
				{
					return *current;
				}
			};
			
			Iterator begin()
			{
				return Iterator(*this, first(), last());
			}
	};
};
