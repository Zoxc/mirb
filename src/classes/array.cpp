#include "array.hpp"
#include "string.hpp"
#include "fixnum.hpp"
#include "range.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Array::allocate(Class *instance_of)
	{
		return Collector::allocate<Array>(instance_of);
	}
	
	value_t Array::unshift(Array *self, size_t argc, value_t argv[])
	{
		self->vector.push_entries_front(argv, argc);

		return self;
	}
	
	value_t Array::push(Array *self, size_t argc, value_t argv[])
	{
		self->vector.push_entries(argv, argc);

		return self;
	}
	
	value_t Array::pop(Array *self)
	{
		if(self->vector.size())
			return self->vector.pop();
		else
			return value_nil;
	}
	
	value_t Array::to_s(Array *self)
	{
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
	
	value_t Array::join(Array *self, String *sep)
	{
		CharArray result;

		OnStack<1> os1(self);
		OnStackString<1> os2(result);

		for(size_t i = 0; i < self->vector.size(); ++i)
		{
			if(i && sep)
				result += sep->string;

			value_t value = self->vector[i];

			auto str = try_cast<String>(value);

			if(str)
				result += str->string;
			else
			{
				auto array = try_cast<Array>(value);

				if(array == self)
					result += "[...]";
				else
				{
					String *desc;

					if(array)
					{
						value_t vsep = sep;
					
						if(sep)
							desc = raise_cast<String>(call(array, "join", 1, &vsep));
						else
							desc = raise_cast<String>(call(array, "join"));
					}
					else
						desc = raise_cast<String>(call(value, "to_s"));

					if(!desc)
						return 0;
					
					result += desc->string;
				}
			}
		}

		return result.to_string();
	}
	
	value_t Array::length(Array *self)
	{
		return Fixnum::from_size_t(self->vector.size());
	}
	
	value_t Array::each(Array *self, value_t block)
	{
		OnStack<2> os(self, block);

		for(size_t i = 0; i < self->vector.size(); ++i)
		{
			if(!yield(block, 1, &self->vector[i]))
				return 0;
		}

		return self;
	}
	
	value_t Array::get(Array *self, value_t index)
	{
		if(Value::is_fixnum(index))
		{
			size_t i = Fixnum::to_size_t(index);

			if(i < self->vector.size())
				return self->vector[i];
			else
				return value_nil;
		}
		else if(Value::of_type<Range>(index))
		{
			size_t start;
			size_t length;

			auto range = cast<Range>(index);

			if(!range->convert_to_index(start, length, self->vector.size()))
				return 0;

			auto result = Collector::allocate<Array>();

			result->vector.push_entries(&self->vector[start], length);

			return result;
		}
		else
			return type_error(index, "Range or Fixnum");
	}
	
	value_t Array::set(Array *self, size_t index, value_t value)
	{
		self->vector.expand_to(index + 1, value_nil);
		self->vector[index] = value;

		return value;
	}
	
	value_t first(Array *self)
	{
		if(self->vector.size() > 0)
			return self->vector[0];
		else
			return value_nil;
	}
	
	value_t last(Array *self)
	{
		if(self->vector.size() > 0)
			return self->vector[self->vector.size() - 1];
		else
			return value_nil;
	}
	
	value_t empty(Array *self)
	{
		return auto_cast(self->vector.size() == 0);
	}
	
	void Array::initialize()
	{
		context->array_class = define_class("Array", context->object_class);
		
		include_module(context->array_class, context->enumerable_module);
		
		singleton_method<Arg::SelfClass<Class>>(context->array_class, "allocate", &allocate);

		method<Arg::SelfClass<Array>>(context->array_class, "first", &first);
		method<Arg::SelfClass<Array>>(context->array_class, "last", &last);
		method<Arg::SelfClass<Array>>(context->array_class, "empty?", &empty);

		method<Arg::SelfClass<Array>, Arg::Count, Arg::Values>(context->array_class, "unshift", &unshift);
		method<Arg::SelfClass<Array>, Arg::Count, Arg::Values>(context->array_class, "push", &push);
		method<Arg::SelfClass<Array>, Arg::Count, Arg::Values>(context->array_class, "<<", &push);
		method<Arg::SelfClass<Array>>(context->array_class, "pop", &pop);
		method<Arg::SelfClass<Array>>(context->array_class, "length", &length);
		method<Arg::SelfClass<Array>, Arg::DefaultClass<String>>(context->array_class, "join", &join);
		method<Arg::SelfClass<Array>>(context->array_class, "to_s", &to_s);
		method<Arg::SelfClass<Array>, Arg::Block>(context->array_class, "each", &each);
		method<Arg::SelfClass<Array>, Arg::Value>(context->array_class, "[]", &get);
		method<Arg::SelfClass<Array>, Arg::UInt, Arg::Value>(context->array_class, "[]=", &set);
	}
};

