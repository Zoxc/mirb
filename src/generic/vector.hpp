#pragma once
#include "allocator.hpp"
#include "../common.hpp"

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
						mirb_debug_assert(_size > 0);

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

			template<typename Aother> void initialize_copy(const Vector<T, typename Aother>& other)
			{
				_size = other.size();
				_capacity = other.capacity();
				
				if(other.raw())
				{
					table = (T *)alloc_ref.alloc(sizeof(T) * _capacity);

					std::memcpy(table, other.raw(), sizeof(T) * _size);
				}
				else
				{
					table = 0;
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
				alloc_ref(vector.alloc_ref),
				_size(vector._size),
				_capacity(vector._capacity)
			{
				if(vector.raw())
				{
					table = (T *)alloc_ref.alloc(sizeof(T) * _capacity);

					std::memcpy(table, vector.table, sizeof(T) * _size);
				}
				else
				{
					table = 0;
				}
			}
			
			template<typename Aother> Vector(const Vector<T, Aother>& other)
			{
				initialize_copy(other);
			}
			
			template<typename Aother> Vector(const Vector<T, Aother>& other, typename A::Ref alloc_ref) : alloc_ref(alloc_ref)
			{
				initialize_copy(other);
			}
			
			~Vector()
			{
				if(table)
					alloc_ref.free((void *)table);
			}
			
			template<typename Aother> Vector operator=(const Vector<T, Aother>& other)
			{
				if(this == &other)
					return *this;
				
				if(table)
					alloc_ref.free((void *)table);

				initialize_copy(other);

				return *this;
			}
			
			size_t size() const
			{
				return _size;
			}
			
			size_t capacity() const
			{
				return _capacity;
			}
			
			const T &first() const
			{
				return *table;
			}
			
			T *raw() const
			{
				return table;
			}
			
			T &first()
			{
				mirb_debug_assert(_size > 0);
				return *table;
			}
			
			T &last()
			{
				mirb_debug_assert(_size > 0);
				return &table[_size - 1];
			}
			
			T &operator [](size_t index)
			{
				mirb_debug_assert(index < _size);
				return table[index];
			}
			
			void push(T entry)
			{
				expand(1);

				table[_size++] = entry;
			}
			
			T pop()
			{
				mirb_debug_assert(_size);
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
				
				T &operator*() const 
				{
					return *current;
				}
				
				T &operator ()() const 
				{
					return *current;
				}
			};
			
			Iterator begin()
			{
				return Iterator(raw());
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
