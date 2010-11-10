#pragma once
#include "allocator.hpp"

namespace Mirb
{
	template<class T, class A = StdLibAllocator> class Vector
	{
		protected:
			T *table;
			typename A::Storage alloc_ref;
			size_t _size;
			size_t _capacity;
		
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
			
			~Vector()
			{
				if(table)
					alloc_ref.free((void *)table);
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
				assert(index < _size);
				return table[index];
			}
			
			void push(T entry)
			{
				if(_size + 1 > _capacity)
				{
					if(table)
					{
						size_t old_capacity = _capacity;
						_capacity <<= 1;
						table = (T *)alloc_ref.realloc((void *)table, sizeof(T) * old_capacity, sizeof(T) * _capacity);
					}
					else
					{
						_capacity = 1;
						table = (T *)alloc_ref.alloc(sizeof(T) * _capacity);
					}
				}
				
				table[_size++] = entry;
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
				/*
				T &operator ++(int)
				{
					return *current++;
				}
				*/
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
	};
};
