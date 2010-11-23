#pragma once
#include "allocator.hpp"
#include "common.hpp"

namespace Mirb
{
	template<class T, class A = StdLibAllocator> class Vector
	{
		protected:
			T *table;
			typename A::Storage alloc_ref;
			size_t _size;
			size_t _capacity;

			void expand(size_t num)
			{
				if(_size + num > _capacity)
				{
					size_t old_capacity = _capacity;

					if(table)
					{
						do
						{
							_capacity <<= 1;
						}
						while(_size + num > _capacity);

						table = (T *)alloc_ref.realloc((void *)table, sizeof(T) * old_capacity, sizeof(T) * _capacity);
					}
					else
					{
						_capacity = 1;

						while(_size + num > _capacity)
							_capacity <<= 1;
						
						table = (T *)alloc_ref.alloc(sizeof(T) * _capacity);
					}
				}
			}
		
		public:
			Vector(size_t initial) : alloc_ref(A::Storage::def_ref())
			{
				_size = 0;
				_capacity = 1 << initial;
				
				table = (T *)alloc_ref.alloc(sizeof(T) * _capacity);
			}
			
			Vector(size_t initial, typename A::Ref alloc_ref) : alloc_ref(alloc_ref)
			{
				_size = 0;
				_capacity = 1 << initial;
				
				table = (T *)alloc_ref.alloc(sizeof(T) * _capacity);
			}
			
			Vector() : alloc_ref(A::Storage::def_ref())
			{
				_size = 0;
				_capacity = 0;
				table = 0;
			}
			
			Vector(typename A::Ref alloc_ref) : alloc_ref(alloc_ref)
			{
				_size = 0;
				_capacity = 0;
				table = 0;
			}
			
			Vector(const Vector &vector) :
				_size(vector._size),
				_capacity(vector._size),
				alloc_ref(vector.alloc_ref)
			{
				table = (T *)alloc_ref.alloc(sizeof(T) * _capacity);

				std::memcpy(table, vector.table, sizeof(T) * _size);
			}
			
			~Vector()
			{
				if(table)
					alloc_ref.free((void *)table);
			}
			
			Vector operator=(const Vector& other)
			{
				if(this == &other)
					return *this;
				
				if(table)
					alloc_ref.free((void *)table);

				_size = other._size;
				_capacity = other._size;
				
				table = (T *)alloc_ref.alloc(sizeof(T) * _capacity);

				std::memcpy(table, other.table, sizeof(T) * _size);

				return *this;
			}
			
			size_t size()
			{
				return _size;
			}
			
			T &first()
			{
				return *table;
			}
			
			T &last()
			{
				return &table[_size - 1];
			}
			
			T operator [](size_t index)
			{
				debug_assert(index < _size);
				return table[index];
			}
			
			void push(T entry)
			{
				expand(1);

				table[_size++] = entry;
			}
			
			T pop()
			{
				debug_assert(_size);
				return table[--_size];
			}
			
			size_t index_of(T entry)
			{
				auto result = find(entry);

				if(!result)
					return (size_t)-1;

				return (size_t)(result - table) / sizeof(T);
			}
			
			T *find(T entry)
			{
				for(auto i = begin(); i != end(); ++i)
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

			public:
				Iterator(T *start) : current(start) {}
				
				bool operator ==(const Iterator &other) const 
				{
					return current == other.current;
				}
				
				bool operator !=(const Iterator &other) const 
				{
					return current != other.current;
				}
			
				T *position() const 
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
				
				T operator*() const 
				{
					return *current;
				}
				
				T operator ()() const 
				{
					return *current;
				}
			};
			
			Iterator begin()
			{
				return Iterator(&first());
			}

			Iterator end()
			{
				return Iterator(&table[_size]);
			}

			size_t index_of(Iterator &iter)
			{
				return (size_t)(iter.position() - table) / sizeof(T);
			}
	};
};
