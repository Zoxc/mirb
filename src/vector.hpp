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
			size_t _size;
			size_t _capacity;
		
		public:
			Vector(size_t initial)
			{
				_size = 0;
				_capacity = 1 << initial;
				
				table = (T *)F::alloc(sizeof(T) * _capacity);
			}
			
			Vector()
			{
				_size = 0;
				_capacity = 0;
				table = 0;
			}
			
			~Vector()
			{
				F::free((void *)table);
			}
			
			size_t size()
			{
				return _size;
			}
			
			T *first()
			{
				return table;
			}
			
			T *last()
			{
				return &table[_size];
			}
			
			void push(T entry)
			{
				if(_size + 1 > _capacity)
				{
					if(table)
					{
						size_t old_capacity = _capacity;
						_capacity <<= 1;
						table = (T *)F::realloc((void *)table, sizeof(T) * old_capacity, sizeof(T) * _capacity);
					}
					else
					{
						_capacity = 1;
						table = (T *)F::alloc(sizeof(T) * _capacity);
					}
				}
				
				table[_size++] = entry;
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
				this->_size = 0;
				this->_capacity = 1 << initial;
				
				this->table = (T *)F::alloc(storage, sizeof(T) * this->_capacity);
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
				if(this->_size + 1 > this->_capacity)
				{
					if(this->table)
					{
						size_t old_capacity = this->_capacity;
						this->_capacity <<= 1;
						this->table = (T *)F::realloc(storage, (void *)this->table, sizeof(T) * old_capacity, sizeof(T) * this->_capacity);
					}
					else
					{
						this->_capacity = 1;
						this->table = (T *)F::alloc(storage, sizeof(T) * this->_capacity);
					}
				}
				
				this->table[this->_size++] = entry;
			}
	};
};
