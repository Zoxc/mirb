#pragma once
#include "common.hpp"
#include "allocator.hpp"

namespace Mirb
{
	template<class K, class V, class A> class HashTableFunctions
	{
		public:
			static size_t hash_key(K key)
			{
				return (size_t)key;
			}

			static bool valid_key(K key)
			{
				return key != 0;
			}

			static bool valid_value(V value)
			{
				return value != 0;
			}

			static V invalid_value()
			{
				return 0;
			}

			static bool create_value()
			{
				return false;
			}

			static V create_value(typename A::Ref alloc_ref, K key)
			{
				return 0;
			}
	};

	template<class K, class V, class T, class A = StdLibAllocator> class HashTable
	{
		private:
			V *table;
			typename A::Storage alloc_ref;
			size_t mask;
			size_t entries;

			V store(V *table, size_t mask, K key, V value)
			{
				size_t index = T::hash_key(key) & mask;
				V entry = table[index];
				V tail = entry;

				while(T::valid_value(entry))
				{
					if(T::compare_key_value(key, entry))
					{
						if(T::valid_value(tail))
						{
							V next = T::get_value_next(entry);
							T::set_value_next(tail, value);
							T::set_value_next(value, next);
						}
						else
						{
							table[index] = value;
							T::set_value_next(value, T::invalid_value());
						}

						return entry;
					}

					tail = entry;
					entry = T::get_value_next(entry);
				}

				if(T::valid_value(tail))
					T::set_value_next(tail, value);
				else
					table[index] = value;

				T::set_value_next(value, T::invalid_value());

				return T::invalid_value();
			}

			void expand()
			{
				size_t size = (this->mask + 1) << 1;
				size_t mask = size - 1;

				V *table = (V *)alloc_ref.alloc(size * sizeof(V));
				std::memset(table, 0, size * sizeof(V));

				V *end = this->table + size;

				for(V *slot = this->table; slot != end; ++slot)
				{
					V entry = *slot;

					while(T::valid_value(entry))
					{
						V next = T::get_value_next(entry);

						store(table, mask, T::get_key(entry), entry);

						entry = next;
					}
				}

				alloc_ref.free(this->table);

				this->mask = mask;
				this->table = table;
			}

			void increase()
			{
				entries++;

				if(entries > mask)
					expand();
			}

			void setup(size_t initial)
			{
				entries = 0;
				
				size_t size = 1 << initial;
				mask = size - 1;

				table = (V *)alloc_ref.alloc(size * sizeof(V));
				memset(table, 0, size * sizeof(V));
			}

		protected:
			V* get_table()
			{
				return table;
			}

			size_t get_size()
			{
				return mask + 1;
			}

		public:
			HashTable(size_t initial) : alloc_ref(A::Storage::def_ref())
			{
				setup(initial);
			}

			HashTable(size_t initial, typename A::Ref alloc_ref) : alloc_ref(alloc_ref)
			{
				setup(initial);
			}

			~HashTable()
			{
				alloc_ref.free(this->table);
			}

			V get(K key)
			{
				if(!T::valid_key(key))
					return 0;

				size_t index = T::hash_key(key) & mask;
				V entry = table[index];
				V tail = entry;

				while(T::valid_value(entry))
				{
					if(T::compare_key_value(key, entry))
						return entry;

					tail = entry;
					entry = T::get_value_next(entry);
				}

				if(T::create_value())
				{
					V value = T::create_value(alloc_ref, key);

					if(tail)
						T::set_value_next(tail, value);
					else
						table[index] = value;

					T::set_value_next(value, T::invalid_value());

					increase();

					return value;
				}
				else
					return T::invalid_value();
			}

			V set(K key, V value)
			{
				V existing = store(table, mask, key, value);

				if(!T::valid_value(existing))
					increase();

				return existing;
			}
	};
};
