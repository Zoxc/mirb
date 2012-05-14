#include "hash.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Hash::allocate(Class *instance_of)
	{
		return Collector::allocate<Hash>(instance_of);
	}

	value_t Hash::get(Hash *self, value_t key)
	{
		return HashAccess::get(self, key, [&]() -> value_t {
			if(self->flag)
				return yield(self->default_value);
			else
				return self->default_value;
		});
	}
	
	value_t Hash::set(Hash *self, value_t key, value_t value)
	{
		OnStack<1> os(value);

		if(!HashAccess::set(self, key, value))
			return 0;

		return value;
	}
	
	value_t Hash::to_s(Hash *self)
	{
		CharArray result = "{";

		OnStack<1> os1(self);
		OnStackString<1> os2(result);

		if(!HashAccess::each_pair(self, [&](value_t key, value_t value) -> bool {

			OnStack<1> os(value);
			
			if(!append_inspect(result, key))
				return false;

			result += "=>";

			if(!append_inspect(result, value))
				return false;

			return true;
		}))
			return 0;

		result += "}";

		return result.to_string();
	}
	
	value_t Hash::each(Hash *self, value_t block)
	{
		OnStack<2> os(self, block);

		if(!HashAccess::each_pair(self, [&](value_t key, value_t value) -> bool {
			value_t argv[2];
			
			argv[0] = key;
			argv[1] = value;
			
			return yield(block, 2, argv) != 0;
		}))
			return 0;

		return self;
	}

	void Hash::initialize()
	{
		context->hash_class = define_class("Hash", context->object_class);
		
		singleton_method<Arg::SelfClass<Class>>(context->hash_class, "allocate", &allocate);
		
		method<Arg::SelfClass<Hash>, Arg::Block>(context->hash_class, "each", &each);

		method<Arg::SelfClass<Hash>>(context->hash_class, "to_s", &to_s);
		method<Arg::SelfClass<Hash>, Arg::Value>(context->hash_class, "[]", &get);
		method<Arg::SelfClass<Hash>, Arg::Value, Arg::Value>(context->hash_class, "[]=", &set);
	}
};

