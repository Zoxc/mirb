#pragma once
#include "common.hpp"
#include <Prelude/Allocator.hpp>
#include <Prelude/LinkedList.hpp>
#include "value.hpp"

namespace Mirb
{
	class Collector;

	// PinnedHeader are objects with a fixed memory address

	class PinnedHeader:
		public Value
	{
		private:
			#ifndef VALGRIND
				LinkedListEntry<PinnedHeader> entry;
			#endif

			friend class Collector;
		public:
			PinnedHeader(Type::Enum type) : Value(type) {}
	};
	
	class VariableBlock:
		public Value
	{
		private:
		public:
			VariableBlock(size_t bytes) : Value(Type::InternalVariableBlock), bytes(bytes) {}

			static VariableBlock &from_memory(const void *memory)
			{
				auto result = (VariableBlock *)((const char_t *)memory - sizeof(VariableBlock));
				
				result->assert_alive();

				mirb_debug_assert(result->type() == Type::InternalVariableBlock);

				return *result;
			}

			void *data()
			{
				return (void *)((size_t)this + sizeof(VariableBlock));
			}
			
			size_t bytes;

			template<typename F> void mark(F)
			{
			}
	};

	class TupleBase
	{
		static Tuple<Value> *allocate_value_tuple(size_t size);
		static Tuple<Object> *allocate_tuple(size_t size);

		template<class T> friend struct TupleUtil;
	};

	template<class T> struct TupleUtil
	{
		static const Type::Enum type = Type::InternalTuple;

		template<typename F> static void mark(F mark, T *&field)
		{
			if(field)
				mark(field);
		}

		static Tuple<T> *allocate(size_t size)
		{
			return (Tuple<T> *)TupleBase::allocate_tuple(size);
		}
	}; 

	template<> struct TupleUtil<Value>
	{
		static const Type::Enum type = Type::InternalValueTuple;

		template<typename F> static void mark(F mark, Value *&field)
		{
			mark(field);
		}

		static Tuple<> *allocate(size_t size)
		{
			return TupleBase::allocate_value_tuple(size);
		}
	};
	
	template<class T> class Tuple:
		public Value
	{
		private:
		public:
			Tuple(size_t entries) : Value(TupleUtil<T>::type), entries(entries) {}

			T *&operator[](size_t index)
			{
				#ifdef DEBUG
					if(this)
						mirb_debug_assert(index < entries);
				#endif

				return ((T **)((size_t)this + sizeof(Tuple)))[index];
			}
			
			size_t entries;

			template<typename F> void mark(F mark)
			{
				for(size_t i = 0; i < entries; ++i)
					TupleUtil<T>::mark(mark, (*this)[i]);
			}
			
			Tuple *copy_and_prepend(T *value)
			{
				Tuple *result = allocate(entries + 1);
				
				for(size_t i = 0; i < entries; ++i)
					(*result)[i + 1] = (*this)[i];

				(*result)[0] = value;

				return result;
			}

			T *first()
			{
				mirb_debug_assert(entries > 0);

				return (*this)[0];
			}
			
			T *last()
			{
				mirb_debug_assert(entries > 0);

				return (*this)[entries - 1];
			}
			
			static Tuple *allocate(size_t size)
			{
				return TupleUtil<T>::allocate(size);
			}
	};
	
	typedef Prelude::Allocator::Standard AllocatorBase;
	
	class AllocatorPrivate
	{
		template<typename T> static Tuple<T> *allocate_tuple(size_t size);

		template<class A, class BaseAllocator> friend class Allocator;
		
		template<class T> struct Null
		{
			static const T value;
		};
	};
	
	template<> struct AllocatorPrivate::Null<value_t>
	{
		static const value_t value;
	};

	template<class T> const T AllocatorPrivate::Null<T>::value = nullptr;
	
	template<class A, class BaseAllocator> class Allocator:
			public BaseAllocator::ReferenceBase
	{
		public:
			typedef BaseAllocator Base;
			
			typename Base::Reference reference()
			{
				return Base::default_reference;
			}
			
			static const bool null_references = true;

			typedef typename std::iterator_traits<A>::value_type ArrayBase;
			typedef Tuple<ArrayBase> TupleObj;

			class Storage
			{
				private:
					Storage(TupleObj *array) : array(array) {}
					
					friend class Allocator;
				public:
					TupleObj *array;

					Storage()
					{
						#ifdef DEBUG
							array = nullptr;
						#endif
					}
					
					Storage &operator =(decltype(nullptr) null prelude_unused)
					{
						mirb_debug_assert(null == nullptr);
						
						array = nullptr;

						return *this;
					}
					
					Storage &operator =(const Storage &other)
					{
						array = other.array;

						return *this;
					}
					
					operator bool() const
					{
						return array != 0;
					}
					
					A &operator [](size_t index) const
					{
						return (*array)[index];
					}
			};
			
			Allocator(typename Base::Reference = Base::default_reference) {}
			Allocator(const Allocator &) {}
				
			void null(A &ref)
			{
				ref = AllocatorPrivate::Null<A>::value;
			}
			
			Storage allocate(size_t size)
			{
				TupleObj &result = *TupleObj::allocate(size);

				for(size_t i = 0; i < size; ++i)
					result[i] = AllocatorPrivate::Null<A>::value;

				return Storage(&result);
			}
			
			Storage reallocate(const Storage &old, size_t old_size, size_t new_size)
			{
				TupleObj &result = *TupleObj::allocate(new_size);

				for(size_t i = 0; i < old_size; ++i)
					result[i] = (*old.array)[i];
				
				for(size_t i = old_size; i < new_size; ++i)
					result[i] = AllocatorPrivate::Null<A>::value;

				return Storage(&result);
			}

			void free(const Storage &) {}
	};

	extern Collector &collector;
};

void *operator new(size_t bytes, Mirb::Collector &) throw();
void operator delete(void *, Mirb::Collector &) throw();
