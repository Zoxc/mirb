#include "array.hpp"
#include "string.hpp"
#include "fixnum.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Array::allocate(value_t obj)
	{
		return auto_cast(Collector::allocate<Array>(obj));
	}

	value_t Array::push(value_t obj, size_t argc, value_t argv[])
	{
		auto self = cast<Array>(obj);

		for(size_t i = 0; i < argc; ++i)
			self->vector.push(argv[i]);

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

		OnStack<2> os1(self);
		OnStackString<1> os2(result);

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
		context->array_class = define_class(context->object_class, "Array", context->object_class);
		
		singleton_method<Arg::Self>(context->array_class, "allocate", &allocate);
		
		static_method<Arg::Self, Arg::Count, Arg::Values>(context->array_class, "push", &push);
		static_method<Arg::Self, Arg::Count, Arg::Values>(context->array_class, "<<", &push);
		static_method<Arg::Self>(context->array_class, "pop", &pop);
		static_method<Arg::Self>(context->array_class, "length", &length);
		static_method<Arg::Self>(context->array_class, "inspect", &inspect);
		static_method<Arg::Self, Arg::Block>(context->array_class, "each", &each);
	}
};

