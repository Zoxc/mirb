#pragma once
#include <cstring>

namespace Mirb
{
	template<class K, class V, class S> class HashTableFunctions
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

			static V create_value(S storage, K key)
			{
				return 0;
			}

			static V *alloc(size_t entries)
			{
				return new V[entries];
			}

			static void free(V *table, size_t entries)
			{
				return new V[entries];
			}
	};

	template<class K, class V, class S, class T> class HashTable
	{
		private:
			V *table;
			S storage;
			size_t size;
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
						return entry;

					tail = entry;
					entry = T::get_value_next(entry);
				}

				if(tail)
					T::set_value_next(tail, value);
				else
					table[index] = value;

				T::set_value_next(value, T::invalid_value());

				return T::invalid_value();
			}

			void expand()
			{
				size_t size = this->size + 1;
				size_t real_size = 1 << size;
				size_t mask = real_size - 1;

				V *table = T::alloc(real_size);
				std::memset(table, 0, real_size * sizeof(V));

				V *end = this->table + (1 << this->size);

				for(V *slot = this->table; slot != end; slot++)
				{
					V entry = *slot;

					while(T::valid_value(entry))
					{
						V next = T::get_value_next(entry);

						store(table, mask, T::get_key(entry), entry);

						entry = next;
					}
				}

				T::free(this->table, 1 << this->size);

				this->size = size;
				this->mask = mask;
				this->table = table;
			}

			void increase()
			{
				entries++;

				if(entries > mask)
					expand();
			}

		protected:
			V* get_table()
			{
				return table;
			}

			size_t get_size()
			{
				return 1 << size;
			}

		public:
			HashTable(size_t initial, S storage) : storage(storage)
			{
				entries = 0;

				size = initial;
				size_t real_size = 1 << size;
				mask = real_size - 1;

				table = new V[real_size];
				memset(table, 0, real_size * sizeof(V));
			}

			~HashTable()
			{
				delete[] table;
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
					V value = T::create_value(storage, key);

					if(tail)
						T::set_value_next(tail, value);
					else
						table[index] = value;

					T::set_value_next(value, T::invalid_value());

					increase();

					return value;
				}
				else
					return 0;
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
