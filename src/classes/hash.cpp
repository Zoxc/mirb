#include "hash.hpp"
#include "string.hpp"
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

		HashAccess::set(self, key, value);

		return value;
	}
	
	value_t Hash::to_s(Hash *self)
	{
		CharArray result = "{";

		OnStack<1> os1(self);
		OnStackString<1> os2(result);

		HashAccess::each_pair(self, [&](value_t key, value_t value) -> bool {

			OnStack<1> os(value);
			
			result += inspect(key);
			result += "=>";
			result += inspect(value);
			result += ", ";

			return true;
		});

		if(result.size() > 1)
			result.shrink(result.size() - 2);

		result += "}";

		return result.to_string();
	}
	
	value_t Hash::each(Hash *self, value_t block)
	{
		OnStack<2> os(self, block);

		HashAccess::each_pair(self, [&](value_t key, value_t value) -> bool {
			value_t argv[2];
			
			argv[0] = key;
			argv[1] = value;
			
			yield_argv(block, 2, argv);
			return true;
		});

		return self;
	}

	void Hash::initialize()
	{
		context->hash_class = define_class("Hash", context->object_class);
		
		singleton_method<Arg::Self<Arg::Class<Class>>, &allocate>(context->hash_class, "allocate");
		
		method<Arg::Self<Arg::Class<Hash>>, Arg::Block, &each>(context->hash_class, "each");

		method<Arg::Self<Arg::Class<Hash>>, &to_s>(context->hash_class, "to_s");
		method<Arg::Self<Arg::Class<Hash>>, Arg::Value, &get>(context->hash_class, "[]");
		method<Arg::Self<Arg::Class<Hash>>, Arg::Value, Arg::Value, &set>(context->hash_class, "[]=");
	}
};

