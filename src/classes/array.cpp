#include "array.hpp"
#include "string.hpp"
#include "fixnum.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Array::allocate(value_t obj)
	{
		return auto_cast(Collector::allocate<Array>(auto_cast(obj)));
	}
	
	value_t Array::unshift(value_t obj, size_t argc, value_t argv[])
	{
		auto self = cast<Array>(obj);

		self->vector.push_entries_front(argv, argc);

		return obj;
	}
	
	value_t Array::push(value_t obj, size_t argc, value_t argv[])
	{
		auto self = cast<Array>(obj);

		self->vector.push_entries(argv, argc);

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

		OnStack<1> os1(self);
		OnStackString<1> os2(result);

		for(size_t i = 0; i < self->vector.size(); ++i)
		{
			String *desc = cast<String>(call(self->vector[i], "inspect"));

			if(!desc)
				return 0;

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
		
		OnStack<2> os(self, block);

		for(size_t i = 0; i < self->vector.size(); ++i)
		{
			if(!yield(block, 1, &self->vector[i]))
				return 0;
		}

		return self;
	}
	
	value_t Array::get(value_t obj, size_t index)
	{
		auto self = cast<Array>(obj);

		if(index < self->vector.size())
			return self->vector[index];
		else
			return value_nil;
	}
	
	value_t Array::set(value_t obj, size_t index, value_t value)
	{
		auto self = cast<Array>(obj);

		self->vector.expand_to(index + 1, value_nil);
		self->vector[index] = value;

		return value;
	}
	
	void Array::initialize()
	{
		context->array_class = define_class("Array", context->object_class);
		
		include_module(context->array_class, context->enumerable_module);
		
		singleton_method<Arg::Self>(context->array_class, "allocate", &allocate);
		
		method<Arg::Self, Arg::Count, Arg::Values>(context->array_class, "unshift", &unshift);
		method<Arg::Self, Arg::Count, Arg::Values>(context->array_class, "push", &push);
		method<Arg::Self, Arg::Count, Arg::Values>(context->array_class, "<<", &push);
		method<Arg::Self>(context->array_class, "pop", &pop);
		method<Arg::Self>(context->array_class, "length", &length);
		method<Arg::Self>(context->array_class, "inspect", &inspect);
		method<Arg::Self, Arg::Block>(context->array_class, "each", &each);
		method<Arg::Self, Arg::UInt>(context->array_class, "[]", &get);
		method<Arg::Self, Arg::UInt, Arg::Value>(context->array_class, "[]=", &set);
	}
};

