#pragma once
#include <cstdlib>

namespace Mirb
{
	class VectorFunctions
	{
		public:
			static void *alloc(size_t bytes)
			{
				return std::malloc(bytes);
			}

			static void *realloc(void *table, size_t old, size_t bytes)
			{
				return std::realloc(table, bytes);
			}

			static void free(void *table)
			{
				return std::free(table);
			}
	};
	
	template<class T, class F = VectorFunctions> class Vector
	{
		protected:
			T *table;
			size_t entries;
			size_t size;
		
		public:
			Vector(size_t initial)
			{
				entries = 0;
				size = 1 << initial;
				
				table = (T *)F::alloc(sizeof(T) * size);
			}
			
			Vector()
			{
				entries = 0;
				size = 0;
				table = 0;
			}
			
			~Vector()
			{
				F::free((void *)table);
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
						table = (T *)F::realloc((void *)table, sizeof(T) * (size >> 1), sizeof(T) * size);
					}
					else
					{
						size = 1;
						table = (T *)F::alloc(sizeof(T) * size);
					}
				}
				
				table[entries++] = entry;
			}
			
			T *find(T entry)
			{
				for(auto i = begin(); i; i++)
				{
					if(*i == entry)
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
				
				T operator*()
				{
					return *current;
				}
				
				T operator ()()
				{
					return *current;
				}
			};
			
			Iterator begin()
			{
				return Iterator(*this, first(), last());
			}
	};
	
	template<class S> class StorageVectorFunctions
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

	template<class T, class F> class StorageVector:
		public Vector<T>
	{
		private:
			typename F::Storage storage;
		
		public:
			StorageVector(size_t initial, typename F::Storage storage) : storage(storage)
			{
				this->entries = 0;
				this->size = 1 << initial;
				
				this->table = (T *)F::alloc(storage, sizeof(T) * this->size);
			}
			
			StorageVector(typename F::Storage storage) : storage(storage)
			{
			}
			
			~StorageVector()
			{
				F::free(storage, (void *)this->table);
			}
			
			void push(T entry)
			{
				if(this->entries + 1 > this->size)
				{
					if(this->table)
					{
						this->size <<= 1;
						this->table = (T *)F::realloc(storage, (void *)this->table, sizeof(T) * (this->size >> 1), sizeof(T) * this->size);
					}
					else
					{
						this->size = 1;
						this->table = (T *)F::alloc(storage, sizeof(T) * this->size);
					}
				}
				
				this->table[this->entries++] = entry;
			}
	};
};
