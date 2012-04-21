#pragma once
#include "allocator.hpp"

namespace Mirb
{
	template<class K, class V> class MapFunctions
	{
		public:
			static size_t hash_key(K key)
			{
				return (size_t)key;
			}

			static V invalid_value()
			{
				return 0;
			}
	};

	template<class K, class V, class A = StdLibAllocator, class T = MapFunctions<K ,V>> class Map
	{
		private:
			struct Pair
			{
				K key;
				V value;
				Pair *next;
			};

			Pair **table;
			typename A::Storage alloc_ref;
			size_t mask;
			size_t entries;
			
			bool store(Pair **table, size_t mask, K key, V value)
			{
				size_t index = T::hash_key(key) & mask;
				Pair *pair = table[index];
				Pair *tail = pair;

				while(pair)
				{
					if(pair->key == key)
					{
						pair->value = value;
						return false;
					}

					tail = pair;
					pair = pair->next;
				}
				
				pair = new (alloc_ref.alloc(sizeof(Pair))) Pair;
				pair->key = key;
				pair->value = value;

				if(tail)
					tail->next = pair;
				else
					table[index] = pair;

				pair->next = 0;

				return true;
			}

			void expand()
			{
				size_t size = (this->mask + 1) << 1;
				size_t mask = size - 1;

				Pair **table = (Pair **)alloc_ref.alloc(size * sizeof(Pair *));
				std::memset(table, 0, size * sizeof(Pair *));

				Pair **end = this->table + (this->mask + 1);

				for(Pair **slot = this->table; slot != end; ++slot)
				{
					Pair *pair = *slot;

					while(pair)
					{
						Pair *next = pair->next;

						store(table, mask, pair->key, pair->value);
						
						alloc_ref.free(pair);

						pair = next;
					}
				}

				alloc_ref.free(this->table);

				this->mask = mask;
				this->table = table;
			}

			void increase()
			{
				entries++;

				if(mirb_unlikely(entries > mask))
					expand();
			}

			void setup(size_t initial)
			{
				entries = 0;

				size_t size = 1 << initial;
				mask = size - 1;

				table = (Pair **)alloc_ref.alloc(size * sizeof(V));
				memset(table, 0, size * sizeof(V));
			}

		public:
			Map(size_t initial) : alloc_ref(A::Storage::def_ref())
			{
				setup(initial);
			}

			Map(size_t initial, typename A::Ref alloc_ref) : alloc_ref(alloc_ref)
			{
				setup(initial);
			}

			~Map()
			{
				if(A::can_free)
				{
					Pair **end = this->table + this->mask + 1;

					for(Pair **slot = this->table; slot != end; ++slot)
					{
						Pair *pair = *slot;

						while(pair)
						{
							Pair *next = pair->next;

							alloc_ref.free(pair);

							pair = next;
						}
					}

					alloc_ref.free(this->table);
				}
			}

			V get(K key)
			{
				size_t index = T::hash_key(key) & mask;
				Pair *pair = table[index];

				while(pair)
				{
					if(pair->key == key)
						return pair->value;
					
					pair = pair->next;
				}

				return T::invalid_value();
			}
			
			template<typename func> V try_get(K key, func fails)
			{
				size_t index = T::hash_key(key) & mask;
				Pair *pair = table[index];

				while(pair)
				{
					if(pair->key == key)
						return pair->value;
					
					pair = pair->next;
				}

				return fails();
			}
			
			V *get_ref(K key)
			{
				size_t index = T::hash_key(key) & mask;
				Pair *pair = table[index];

				while(pair)
				{
					if(pair->key == key)
						return &pair->value;
					
					pair = pair->next;
				}

				return 0;
			}
			
			bool has(K key)
			{
				size_t index = T::hash_key(key) & mask;
				Pair *pair = table[index];

				while(pair)
				{
					if(pair->key == key)
						return true;
					
					pair = pair->next;
				}

				return false;
			}

			void set(K key, V value)
			{
				if(store(table, mask, key, value))
					increase();
			}
	};
};
