#include "array.hpp"
#include "string.hpp"
#include "fixnum.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Array::class_ref;
	
	value_t Array::allocate(value_t obj)
	{
		return auto_cast(new (gc) Array(obj));
	}

	value_t Array::push(value_t obj, size_t argc, value_t argv[])
	{
		auto self = cast<Array>(obj);

		MIRB_ARG_EACH(i)
		{
			self->vector.push(argv[i]);
		}

		return obj;
	}
	
	value_t Array::pop(value_t obj)
	{
		auto self = cast<Array>(obj);

		if(self->vector.size())
			return self->vector.pop();
		else
			return value_nil;
	}
	
	value_t Array::inspect(value_t obj)
	{
		auto self = cast<Array>(obj);

		CharArray result = "[";

		OnStack<2> os(self, result);

		for(size_t i = 0; i < self->vector.size(); ++i)
		{
			String *desc = cast<String>(call(self->vector[i], "inspect"));

			result += desc->string;

			if(i != self->vector.size() - 1)
				result += ", ";
		}

		result += "]";

		return result.to_string();
	}
	
	value_t Array::length(value_t obj)
	{
		auto self = cast<Array>(obj);

		return Fixnum::from_size_t(self->vector.size());
	}
	
	value_t Array::each(value_t obj, value_t block)
	{
		auto self = cast<Array>(obj);
		
		OnStack<1> os(self);

		for(auto i = self->vector.begin(); i != self->vector.end(); ++i)
			yield(block, 1, &(*i));

		return auto_cast(self);
	}

	void Array::initialize()
	{
		Array::class_ref = define_class(Object::class_ref, "Array", Object::class_ref);
		
		singleton_method<Arg::Self>(Array::class_ref, "allocate", &allocate);
		
		static_method<Arg::Self, Arg::Count, Arg::Values>(Array::class_ref, "push", &push);
		static_method<Arg::Self, Arg::Count, Arg::Values>(Array::class_ref, "<<", &push);
		static_method<Arg::Self>(Array::class_ref, "pop", &pop);
		static_method<Arg::Self>(Array::class_ref, "length", &length);
		static_method<Arg::Self>(Array::class_ref, "inspect", &inspect);
		static_method<Arg::Self, Arg::Block>(Array::class_ref, "each", &each);
	}
};

