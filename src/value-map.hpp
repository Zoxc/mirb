#pragma once
#include "allocator.hpp"
#include "on-stack.hpp"
#include "finally.hpp"

namespace Mirb
{
	struct ValueMapData
	{
		Tuple<> *table;
		size_t mask;
		size_t entries;
		
		ValueMapData(size_t initial)
		{
			entries = 0;
			mask = initial - 1;

			initial = initial << 1;

			table = Tuple<>::allocate(initial);
			
			for(size_t i = 0; i < initial; ++i)
				(*table)[i] = value_undef;
		}
		
		ValueMapData(const ValueMapData &other) :
			mask(other.mask),
			entries(other.entries)
		{
			size_t size = (mask + 1) << 1;

			table = Tuple<>::allocate(size);
			
			for(size_t i = 0; i < size; ++i)
				(*table)[i] = (*other.table)[i];
		}
		
		template<typename F> void mark(F mark)
		{
			mark(table);
		}
		
		static value_t call(value_t obj, value_t *key);
		static value_t raise();
	};
	
	template<bool light, class Owner, ValueMapData Owner::*field> class ValueMapManipulator
	{
		private:
			static ValueMapData &data(Owner *&owner)
			{
				return owner->*field;
			}

			static ValueMapData &data(ValueMapData &owner)
			{
				return owner;
			}
			
			template<typename O> static void remove_index(O &owner, value_t &key, size_t index)
			{
				size_t hash = hash_value(key);
				size_t free = index;
				size_t mask = data(owner).mask;
				size_t current = index;

				do
				{
					current = (current + 1) & mask;

					mirb_debug_assert(current != index);  // Detect loops in case the free slot is gone

					value_t current_key = key_slot(owner, current);

					if(current_key == value_undef)
					{
						key_slot(owner, free) = value_undef;
						value_slot(owner, free) = value_undef;
						return;
					}
					
					if(hash_value(current_key) == hash)
					{
						key_slot(owner, free) = current_key;
						value_slot(owner, free) = value_slot(owner, current);

						free = current;
					}
				} while(true);
			}

			template<typename O, typename Free, typename Match> static value_t search(O &owner, value_t &key, Free free, Match match)
			{
				size_t mask = data(owner).mask;
				size_t current = hash_value(key) & mask;
				
				mirb_debug_assert(mask >= data(owner).entries);
				
				while(true)
				{
					size_t index = current << 1;

					value_t slot = key_slot(owner, index);

					if(slot == value_undef)
						return free(index);

					if(light)
					{
						if(slot == key)
							return match(index);
					}
					else
					{
						value_t test = ValueMapData::call(slot, &key);

						if(test->test())
							return match(index);
					}

					current = (current + 1) & mask;
				}
			}

			template<typename O> static value_t &key_slot(O &owner, size_t index)
			{
				return (*ValueMapManipulator::data(owner).table)[index];
			}
			
			template<typename O> static value_t &value_slot(O &owner, size_t index)
			{
				return (*ValueMapManipulator::data(owner).table)[index + 1];
			}

			template<bool can_expand, typename O> static value_t store(O &owner, value_t &key, value_t &value)
			{
				return search(owner, key, [&](size_t index) -> value_t {
					ValueMapManipulator::key_slot(owner, index) = key;
					ValueMapManipulator::value_slot(owner, index) = value;
					ValueMapManipulator::data(owner).entries++;
					
					size_t mask = ValueMapManipulator::data(owner).mask;

					if(prelude_unlikely(can_expand && (ValueMapManipulator::data(owner).entries > mask)))
						return ValueMapManipulator::expand(owner);
					else
						return value_false;
				}, [&](size_t index) -> value_t {
					ValueMapManipulator::value_slot(owner, index) = value;
					return value_true;
				});
			}
			
			static value_t expand(ValueMapData &)
			{
				mirb_runtime_abort("Should not be called");
			}

			static value_t expand(Owner *&owner)
			{
				size_t size = data(owner).mask + 1;

				size_t old_entries = data(owner).entries;

				ValueMapData new_data(size << 1);

				if(light)
				{
					for(size_t i = 0; i < size; ++i)
					{
						value_t key = key_slot(owner, i << 1);

						if(key != value_undef)
						{
							value_t value = value_slot(owner, i << 1);

							if(!store<false>(new_data, key, value))
								return 0;
						}
					}
				}
				else
				{
					OnStack<1> os(new_data.table);

					for(size_t i = 0; i < size; ++i)
					{
						value_t key = key_slot(owner, i << 1);

						if(key != value_undef)
						{
							value_t value = value_slot(owner, i << 1);

							OnStack<2> os2(key, value);

							if(!store<false>(new_data, key, value))
								return 0;
						}
					}
				}

				data(owner) = new_data;
				
				mirb_debug_assert(old_entries == data(owner).entries);
				mirb_debug_assert(data(owner).mask >= data(owner).entries);

				return value_true;
			}
			
			template<typename Arg1, typename F> static value_t enter(Owner *&owner, Arg1 &&arg1, F func)
			{
				if(light)
					return func();
				else
				{
					if(owner->accessing())
						return ValueMapData::raise();
				
					owner->accessing(true);
					
					OnStack<2> os(owner, std::forward<Arg1>(arg1));

					value_t result = func();

					owner->accessing(false);

					return result;
				}
			}
			
			template<typename Arg1, typename Arg2, typename F> static value_t enter(Owner *&owner, Arg1 &&arg1, Arg2 &&arg2, F func)
			{
				if(light)
					return func();
				else
				{
					if(owner->accessing())
						return ValueMapData::raise();
				
					owner->accessing(true);

					Finally finally([&] {
						owner->accessing(false);
					});
					
					OnStack<3> os(owner, std::forward<Arg1>(arg1), std::forward<Arg2>(arg2));

					value_t result = func();

					return result;
				}
			}
			
		public:
			template<typename func> static bool each_pair(Owner *&owner, func do_for_pair)
			{
				size_t size = data(owner).mask + 1;

				for(size_t i = 0; i < size; ++i)
				{
					value_t key = key_slot(owner, i << 1);

					if(key != value_undef)
					{
						if(!do_for_pair(key, value_slot(owner, i << 1)))
							return false;
					}
				}
				
				return true;
			}
		
			static value_t remove(Owner *owner, value_t key)
			{
				return ValueMapManipulator::enter(owner, key, [&]() -> value_t {
					return ValueMapManipulator::search(owner, key, [&](size_t) {
						return value_undef;
					}, [&](size_t index) {
						value_t result = ValueMapManipulator::value_slot(owner, index);

						ValueMapManipulator::remove_index(owner, key, index);

						return result;
					});
				});
			}
			
			template<typename func> static value_t get(Owner *owner, value_t key, func create_value)
			{
				return ValueMapManipulator::enter(owner, key, [&]() -> value_t {
					return ValueMapManipulator::search(owner, key, [&](size_t) {
						if(!light)
							owner->accessing(false);

						return create_value();
					}, [&](size_t index) {
						return ValueMapManipulator::value_slot(owner, index);
					});
				});
			}
			
			static value_t has(Owner *owner, value_t key)
			{
				return ValueMapManipulator::enter(owner, key, [&]() -> value_t {
					return ValueMapManipulator::search(owner, key, [&](size_t) {
						return value_false;
					}, [&](size_t) {
						return value_true;
					});
				});
			}
			
			static value_t set(Owner *owner, value_t key, value_t value)
			{
				return ValueMapManipulator::enter(owner, key, value, [&]() -> value_t {
					return ValueMapManipulator::store<true>(owner, key, value);
				});
			}
	};
	
	class ValueMap:
		public Value
	{
		public:
			ValueMap() : Value(Type::InternalValueMap), data(8) {}
			
			ValueMapData data;

			bool accessing(bool = true)
			{
				mirb_runtime_abort("Should not be called");
			}
			
			template<typename F> void mark(F mark)
			{
				data.mark(mark);
			}
	};
	
	typedef ValueMapManipulator<true, ValueMap, &ValueMap::data> ValueMapAccess;
};
