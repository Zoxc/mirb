#include "hash.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Hash::allocate(value_t obj)
	{
		return auto_cast(Collector::allocate<Hash>(auto_cast(obj)));
	}

	value_t Hash::get(value_t obj, value_t key)
	{
		auto self = cast<Hash>(obj);

		return HashAccess::get(self, key, [&]() -> value_t {
			if(self->flag)
				return yield(self->default_value);
			else
				return self->default_value;
		});
	}
	
	value_t Hash::set(value_t obj, value_t key, value_t value)
	{
		auto self = cast<Hash>(obj);

		HashAccess::set(self, key, value);

		return value;
	}
	
	value_t Hash::inspect(value_t obj)
	{
		auto self = cast<Hash>(obj);

		CharArray result = "{";

		OnStack<1> os1(self);
		OnStackString<1> os2(result);

		if(!self->data.each_pair([&](value_t key, value_t value) -> bool {

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
	
	void Hash::initialize()
	{
		context->hash_class = define_class("Hash", context->object_class);
		
		singleton_method<Arg::Self>(context->hash_class, "allocate", &allocate);

		method<Arg::Self>(context->hash_class, "inspect", &inspect);
		method<Arg::Self, Arg::Value>(context->hash_class, "[]", &get);
		method<Arg::Self, Arg::Value, Arg::Value>(context->hash_class, "[]=", &set);
	}
};

