#include "array.hpp"
#include "string.hpp"
#include "fixnum.hpp"
#include "range.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Array::get(size_t index)
	{
		return vector[index];
	}
	
	size_t Array::size()
	{
		return vector.size();
	}
	
	value_t Array::rb_delete(Array *array, value_t obj)
	{
		OnStack<2> os(array, obj);

		bool result = false;

		for(size_t i = array->vector.size(); i-- > 0;)
		{
			value_t test = call_argv(obj, "==", 1, &array->vector[i]);

			if(Value::test(test))
			{
				result = true;
				array->vector.remove(i);
			}
		}

		if(result)
			return obj;
		else
			return value_nil;
	}

	Array *Array::allocate_pair(value_t left, value_t right)
	{
		auto result = Collector::allocate<Array>(context->array_class);

		result->vector.expand(2);
		
		result->vector.push(left);
		result->vector.push(right);

		return result;
	}

	value_t Array::allocate(Class *instance_of)
	{
		return Collector::allocate<Array>(instance_of);
	}
	
	value_t Array::shift(Array *self)
	{
		if(self->vector.size())
			return self->vector.shift();
		else
			return value_nil;
	}
	
	value_t Array::unshift(Array *self, size_t argc, value_t argv[])
	{
		self->vector.push_entries_front(argv, argc);

		return self;
	}
	
	value_t Array::add(Array *self, Array *other)
	{
		auto result = new (collector) Array(*self);
		
		result->vector.push(other->vector);

		return result;
	}
	
	value_t Array::sub(Array *self, Array *other)
	{
		auto result = new (collector) Array;

		OnStack<3> os(result, self, other);

		for(size_t i = 0; i < self->vector.size(); ++i)
		{
			bool found = false;

			for(size_t j = 0; j < other->vector.size(); ++j)
			{
				value_t result = call_argv(self->vector[i], context->syms.equal, 1, &other->vector[j]);

				if(!result)
					return 0;

				if(Value::test(result))
				{
					found = true;
					break;
				}
			}

			if(!found)
				result->vector.push(self->vector[i]);
		}
		
		return result;
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
			CharArray desc = inspect(self->vector[i]);

			result += desc;

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
							desc = raise_cast<String>(call_argv(array, "join", 1, &vsep)); // TODO: Replace with variadic call
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
			yield_argv(block, 1, &self->vector[i]);
		}

		return self;
	}
	
	value_t Array::rb_get(Array *self, value_t index, value_t size)
	{
		if(size)
		{
			if(!Value::is_fixnum(index) || !Value::is_fixnum(size))
				raise(context->type_error, "Arguments must be instances of Fixnum");

			intptr_t start = Fixnum::to_size_t(index);
			intptr_t length = Fixnum::to_size_t(size);
			size_t vector_size = self->vector.size();

			if(start < 0)
			{
				start = -start;

				if(prelude_unlikely((size_t)start > vector_size))
					return value_nil;
				else
					start = vector_size - (size_t)start;
			}

			if(prelude_unlikely(length < 1))
				return value_nil;

			auto result = Collector::allocate<Array>();
			
			if((size_t)start < self->vector.size())
			{
				if((size_t)start + (size_t)length > vector_size)
					length = vector_size - start;

				result->vector.push_entries(&self->vector[start], length);
			}

			return result;
		}

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

			range->convert_to_index(start, length, self->vector.size());

			auto result = Collector::allocate<Array>();

			result->vector.push_entries(&self->vector[start], length);

			return result;
		}
		else
			type_error(index, "Range or Fixnum");
	}
	
	value_t Array::rb_set(Array *self, size_t index, value_t value)
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
		return Value::from_bool(self->vector.size() == 0);
	}
	
	bool Array::sort(Array *&array)
	{
		size_t pos = 1;

		while(pos < array->vector.size())
		{
			value_t result = compare(array->vector[pos], array->vector[pos - 1]);

			if(!result)
				return false;

			if(Fixnum::to_int(result) >= 0)
				++pos;
			else
			{
				std::swap(array->vector[pos], array->vector[pos - 1]);

				if(pos > 1)
					--pos;
				else
					++pos;
			}
		}

		return true;
	}

	value_t Array::rb_sort(Array *self)
	{
		auto result = new (collector) Array;

		result->vector.push_entries(self->vector.raw(), self->vector.size());
		
		OnStack<1> os(result);

		if(!sort(result))
			return 0;

		return result;
	}
	
	void Array::initialize()
	{
		context->array_class = define_class("Array", context->object_class);
		
		include_module(context->array_class, context->enumerable_module);
		
		singleton_method<Arg::Self<Arg::Class<Class>>>(context->array_class, "allocate", &allocate);
		
		method<Arg::Self<Arg::Class<Array>>>(context->array_class, "sort", &rb_sort);
		
		method<Arg::Self<Arg::Class<Array>>, Arg::Value>(context->array_class, "delete", &rb_delete);

		method<Arg::Self<Arg::Class<Array>>>(context->array_class, "first", &first);
		method<Arg::Self<Arg::Class<Array>>>(context->array_class, "last", &last);
		method<Arg::Self<Arg::Class<Array>>>(context->array_class, "empty?", &empty);
		
		method<Arg::Self<Arg::Class<Array>>>(context->array_class, "shift", &shift);
		method<Arg::Self<Arg::Class<Array>>, Arg::Count, Arg::Values>(context->array_class, "unshift", &unshift);
		method<Arg::Self<Arg::Class<Array>>, Arg::Count, Arg::Values>(context->array_class, "push", &push);
		method<Arg::Self<Arg::Class<Array>>, Arg::Class<Array>>(context->array_class, "+", &add);
		method<Arg::Self<Arg::Class<Array>>, Arg::Class<Array>>(context->array_class, "-", &sub);
		method<Arg::Self<Arg::Class<Array>>, Arg::Count, Arg::Values>(context->array_class, "<<", &push);
		method<Arg::Self<Arg::Class<Array>>>(context->array_class, "pop", &pop);
		method<Arg::Self<Arg::Class<Array>>>(context->array_class, "length", &length);
		method<Arg::Self<Arg::Class<Array>>>(context->array_class, "size", &length);
		method<Arg::Self<Arg::Class<Array>>, Arg::Default<Arg::Class<String>>>(context->array_class, "join", &join);
		method<Arg::Self<Arg::Class<Array>>>(context->array_class, "to_s", &to_s);
		method<Arg::Self<Arg::Class<Array>>, Arg::Block>(context->array_class, "each", &each);
		method<Arg::Self<Arg::Class<Array>>, Arg::Value, Arg::Default<Arg::Value>>(context->array_class, "[]", &rb_get);
		method<Arg::Self<Arg::Class<Array>>, Arg::UInt, Arg::Value>(context->array_class, "[]=", &rb_set);
	}
};

