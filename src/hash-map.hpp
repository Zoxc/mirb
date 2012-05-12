#pragma once
#include "allocator.hpp"
#include "runtime.hpp"

namespace Mirb
{
	struct HashMapData
	{
		Tuple<> *table;
		size_t mask;
		size_t entries;
		
		HashMapData(size_t initial)
		{
			entries = 0;
			mask = initial - 1;

			initial = initial << 1;

			table = Tuple<>::allocate(initial);
			
			for(size_t i = 0; i < initial; ++i)
				(*table)[i] = value_undef;
		}
		
		template<typename F> void mark(F mark)
		{
			mark(table);
		}
	};

	template<bool light, class Owner, HashMapData Owner::*field> class HashMapManipulator
	{
		private:
			static HashMapData &data(Owner *&owner)
			{
				return owner->*field;
			}

			static HashMapData &data(HashMapData &owner)
			{
				return owner;
			}

			template<typename O, typename Free, typename Match> static value_t search(O owner, value_t &key, Free free, Match match)
			{
				size_t mask = data(owner).mask;
				size_t current = Value::hash(key) & mask;
				
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
						value_t test = call(slot, "==", value_nil, 1, &key);

						if(!test)
							return 0;

						if(Value::test(test))
							return match(index);
					}

					current = (current + 1) & mask;
				}
			}

			template<typename O> static value_t &key_slot(O owner, size_t index)
			{
				return (*data(owner).table)[index];
			}
			
			template<typename O> static value_t &value_slot(O owner, size_t index)
			{
				return (*data(owner).table)[index + 1];
			}

			template<bool can_expand, typename O> static value_t store(O owner, value_t &key, value_t &value)
			{
				return search(owner, key, [&](size_t index) -> value_t {
					key_slot(owner, index) = key;
					value_slot(owner, index) = value;
					data(owner).entries++;
					
					size_t mask = data(owner).mask;

					if(prelude_unlikely(can_expand && (data(owner).entries > mask)))
						return expand(owner);
					else
						return value_false;
				}, [&](size_t index) -> value_t {
					value_slot(owner, index) = value;
					return value_true;
				});
			}
			
			static value_t expand(HashMapData &)
			{
				mirb_runtime_abort("Should not be called");
			}

			static value_t expand(Owner *&owner)
			{
				size_t size = data(owner).mask + 1;

				size_t old_entries = data(owner).entries;

				HashMapData new_data(size << 1); // TODO: OnStack for this
				
				for(size_t i = 0; i < size; i += 1)
				{
					value_t key = key_slot(owner, i << 1);

					if(key != value_undef)
					{
						value_t value = value_slot(owner, i << 1); // TODO: OnStack for key, value

						if(!store<false, HashMapData &>(new_data, key, value))
							return 0;
					}
				}

				data(owner) = new_data;
				
				mirb_debug_assert(old_entries == data(owner).entries);
				mirb_debug_assert(data(owner).mask >= data(owner).entries);

				return value_true;
			}

			static value_t increase(Owner *&owner)
			{
				size_t mask = data(owner).mask;

				if(prelude_unlikely(data(owner).entries > (mask - 1 - (mask >> 2)) ))
					return expand(owner);
				
				return value_true;
			}
			
			template<typename Arg1, typename F> static value_t enter(Owner *&owner, Arg1 &&arg1, F func)
			{
				if(light)
					return func();
				else
				{
					if(owner->accessing())
						return raise(context->runtime_error, "Recursive function calls");
				
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
						return raise(context->runtime_error, "Recursive function call");
				
					owner->accessing(true);
					
					OnStack<3> os(owner, std::forward<Arg1>(arg1), std::forward<Arg2>(arg2));

					value_t result = func();

					owner->accessing(false);

					return result;
				}
			}
			
		public:
			template<typename func> static value_t get(Owner *owner, value_t key, func create_value)
			{
				return enter(owner, key, [&]() -> value_t {
					return search(owner, key, [&](size_t) {
						return create_value();
					}, [&](size_t index) {
						return value_slot(owner, index);
					});
				});
			}
			
			static value_t has(Owner *owner, value_t key)
			{
				return enter(owner, key, [&]() -> value_t {
					return search(owner, key, [&](size_t) {
						return value_false;
					}, [&](value_t &result, size_t) {
						return value_true;
					});
				});
			}
			
			static value_t set(Owner *owner, value_t key, value_t value)
			{
				return enter(owner, key, value, [&]() -> value_t {
					return store<true>(owner, key, value);
				});
			}
	};
	
	class HashMap:
		public Value::Header
	{
		public:
			HashMap() : Value::Header(Value::InternalHashMap), data(8) {}
			
			HashMapData data;

			bool accessing(bool = true)
			{
				mirb_runtime_abort("Should not be called");
			}
			
			template<typename F> void mark(F mark)
			{
				data.mark(mark);
			}
	};
	
	typedef HashMapManipulator<true, HashMap, &HashMap::data> HashMapAccess;
};
