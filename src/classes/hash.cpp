#include "hash.hpp"
#include "string.hpp"
#include "array.hpp"
#include "bignum.hpp"
#include "../runtime.hpp"
#include "../recursion-detector.hpp"

namespace Mirb
{
	value_t Hash::allocate(Class *instance_of)
	{
		return Collector::allocate<Hash>(instance_of);
	}
	
	Hash *Hash::dup(Hash *other)
	{
		if(other->accessing())
			raise(context->runtime_error, "Cannot duplicate hash during operations");

		return new (collector) Hash(*other);
	}
			
	value_t Hash::get_default(value_t key)
	{
		if(flag)
			return yield(this->default_value, this, key);
		else
			return default_value;
	}

	value_t Hash::rb_delete(Hash *self, value_t key)
	{
		OnStack<2> os(self, key);

		value_t value = HashAccess::remove(self, key);

		if(value != value_undef)
			return value;
		else
			return self->get_default(key);
	}

	value_t Hash::get(Hash *self, value_t key)
	{
		OnStack<2> os(self, key);

		return HashAccess::get(self, key, [&]() -> value_t {
			return self->get_default(key);
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
		RecursionDetector<RecursionType::Hash_to_s, false> rd(self);

		if(rd.recursion())
			return String::get("{...}");

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
			yield(block, key, value);
			return true;
		});

		return self;
	}
	
	value_t Hash::keys(Hash *self)
	{
		auto array = Array::allocate();

		HashAccess::each_pair(self, [&](value_t key, value_t) -> bool {
			array->vector.push(key);

			return true;
		});

		return array;
	}
	
	value_t Hash::values(Hash *self)
	{
		auto array = Array::allocate();

		HashAccess::each_pair(self, [&](value_t, value_t value) -> bool {
			array->vector.push(value);

			return true;
		});

		return array;
	}

	value_t Hash::rb_initialize(Hash *obj, value_t def, value_t block)
	{
		if(def)
		{
			obj->default_value = def;
		}
		else if(block)
		{
			obj->default_value = block;
			obj->flag = true;
		}

		return value_nil;
	}
	
	value_t Hash::rb_empty(Hash *self)
	{
		return Value::from_bool(self->data.entries == 0);
	}
	
	void Hash::initialize()
	{
		context->hash_class = define_class("Hash", context->object_class);
		
		singleton_method<Self<Class>, &allocate>(context->hash_class, "allocate");

		method<Self<Hash>, Optional<Value>, Arg::Block, &rb_initialize>(context->hash_class, "initialize");
		
		method<Self<Hash>, &rb_empty>(context->hash_class, "empty?");
		method<Self<Hash>, Value, &rb_delete>(context->hash_class, "delete");
		method<Self<Hash>, &keys>(context->hash_class, "keys");
		method<Self<Hash>, &values>(context->hash_class, "values");

		method<Self<Hash>, Arg::Block, &each>(context->hash_class, "each");

		method<Self<Hash>, &to_s>(context->hash_class, "to_s");
		method<Self<Hash>, Value, &get>(context->hash_class, "[]");
		method<Self<Hash>, Value, Value, &set>(context->hash_class, "[]=");
	}
};

