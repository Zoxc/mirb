#include "array.hpp"
#include "string.hpp"
#include "fixnum.hpp"
#include "range.hpp"
#include "../recursion-detector.hpp"
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
	
	value_t Array::rb_clear(Array *array)
	{
		array->vector.clear();

		return array;
	}

	value_t Array::rb_delete(Array *array, value_t obj)
	{
		OnStack<1> os(obj);

		bool result = delete_if(array, [&](value_t &value) {
			return call(obj, "==", value)->test();
		});

		if(result)
			return obj;
		else
			return value_nil;
	}

	Array *Array::allocate_pair(value_t left, value_t right)
	{
		auto result = allocate();

		result->vector.expand(2);
		
		result->vector.push(left);
		result->vector.push(right);

		return result;
	}
	
	Array *Array::allocate()
	{
		return Collector::allocate<Array>(context->array_class);
	}

	value_t Array::rb_allocate(Class *instance_of)
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
	
	value_t Array::concat(Array *self, Array *other)
	{
		self->vector.push(other->vector);

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
				value_t result = call(self->vector[i], context->syms.equal, other->vector[j]);

				if(result->test())
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

		RecursionDetector<RecursionType::Array_to_s, false> rd(self);

		if(rd.recursion())
			return String::get("[...]");

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
	
	void Array::flatten(VectorType &vector, bool &mod)
	{
		RecursionDetector<RecursionType::Array_flatten> rd(this);

		for(size_t i = 0; i < size(); ++i)
		{
			auto array = try_cast<Array>(get(i));

			if(array)
			{
				mod = true;
				array->flatten(vector, mod);
			}
			else
				vector.push(get(i));
		}
	}

	value_t Array::rb_flatten(Array *self)
	{
		auto result = Array::allocate();

		bool mod;

		self->flatten(result->vector, mod);

		return result;
	}

	value_t Array::rb_flatten_ex(Array *self)
	{
		bool mod;

		VectorType new_vector;

		self->flatten(new_vector, mod);

		self->vector = std::move(new_vector);

		return mod ? self : value_nil;
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
				
				RecursionDetector<RecursionType::Array_join, false> rd(array);

				if(rd.recursion())
				{
					result += "[...]";
					continue;
				}

				String *desc;

				if(array)
				{
					if(sep)
						desc = raise_cast<String>(call(array, "join", sep));
					else
						desc = raise_cast<String>(call(array, "join"));
				}
				else
					desc = raise_cast<String>(call(value, "to_s"));

				result += desc->string;
			}
		}

		return result.to_string();
	}
	
	value_t Array::equal(Array *self, value_t other)
	{
		auto array = try_cast<Array>(other);

		if(!array)
			return value_false;

		if(self->size() != array->size())
			return value_false;
		
		RecursionDetector<RecursionType::Array_equal> rd(self);

		OnStack<2> os(self, array);

		for(size_t i = 0; i < self->vector.size(); ++i)
		{
			if(i >= array->size()) // Check for when the size has been modified
				return value_false;

			if(!call(self->vector[i], "==", array->vector[i])->test())
				return value_false;
		}

		return Value::from_bool(self->size() == array->size()); // Check for when the size has been modified
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
			yield(block, self->vector[i]);
		}

		return self;
	}
	
	value_t Array::reverse_each(Array *self, value_t block)
	{
		OnStack<2> os(self, block);

		for(size_t i = self->vector.size(); i-- > 0;)
		{
			if(i >= self->vector.size())
				return self;

			yield(block, self->vector[i]);
		}

		return self;
	}
	
	value_t Array::rb_get(Array *self, value_t index, value_t size)
	{
		if(size)
		{
			size_t start;

			if(!map_index(Fixnum::to_int(index), self->vector.size(), start))
				return value_nil;
			
			auto length = Fixnum::to_int(raise_cast<Type::Fixnum>(size));

			if(length < 0)
				return value_nil;

			length = std::min((size_t)length, self->vector.size() - start);

			auto result = Collector::allocate<Array>();
			
			result->vector.push_entries(&self->vector[start], length);

			return result;
		}

		if(Value::is_fixnum(index))
		{
			size_t i;

			if(!map_index(Fixnum::to_int(index), self->vector.size(), i))
				return value_nil;

			return self->vector[i];
		}
		else if(of_type<Range>(index))
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
			int result = compare(array->vector[pos], array->vector[pos - 1]);

			if(result >= 0)
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

		sort(result);

		return result;
	}
	
	value_t Array::reject_ex(Array *self, value_t block)
	{
		OnStack<1> os(block);

		return Value::from_bool(delete_if(self, [&](value_t &value) {
			return yield(block, value)->test();
		}));
	}
	
	value_t Array::reject(Array *self, value_t block)
	{
		OnStack<1> os(block);

		auto result = Array::dup(self);

		delete_if(result, [&](value_t &value) {
			return yield(block, value)->test();
		});

		return result;
	}
	
	value_t Array::compact_ex(Array *self)
	{
		return Value::from_bool(delete_if(self, [&](value_t value) {
			return value == value_nil;
		}));
	}
	
	value_t Array::compact(Array *self)
	{
		auto result = Array::dup(self);

		delete_if(result, [&](value_t value) {
			return value == value_nil;
		});

		return result;
	}
	
	value_t Array::replace(Array *self, Array *other)
	{
		if(self != other)
			self->vector = other->vector;

		return self;
	}
	
	value_t Array::values_at(Array *self, size_t argc, value_t argv[])
	{
		auto result = allocate();

		for(size_t i = 0; i < argc; ++i)
		{
			value_t index = argv[i];
			
			if(Value::is_fixnum(index))
			{
				size_t i;

				if(!map_index(Fixnum::to_int(index), self->vector.size(), i))
					continue;

				result->vector.push(self->vector[i]);
			}
			else if(of_type<Range>(index))
			{
				size_t start;
				size_t length;

				auto range = cast<Range>(index);

				range->convert_to_index(start, length, self->vector.size());

				result->vector.push_entries(&self->vector[start], length);
			}
			else
				type_error(index, "Range or Fixnum");
		}
		
		return result;
	}

	void Array::initialize()
	{
		context->array_class = define_class("Array", context->object_class);
		
		include_module(context->array_class, context->enumerable_module);
		
		method<Self<Array>, Array, &concat>(context->array_class, "concat");
		method<Self<Array>, Array, &replace>(context->array_class, "replace");

		method<Self<Array>, &compact_ex>(context->array_class, "compact!");
		method<Self<Array>, &compact>(context->array_class, "compact");
		method<Self<Array>, Arg::Block, &reject>(context->array_class, "reject");
		method<Self<Array>, Arg::Block, &reject_ex>(context->array_class, "reject!");
		method<Self<Array>, Arg::Block, &reject_ex>(context->array_class, "delete_if");

		singleton_method<Self<Class>, &rb_allocate>(context->array_class, "allocate");
		
		method<Self<Array>, &rb_sort>(context->array_class, "sort");
		
		method<Self<Array>, &rb_clear>(context->array_class, "clear");
		method<Self<Array>, Value, &rb_delete>(context->array_class, "delete");

		method<Self<Array>, &first>(context->array_class, "first");
		method<Self<Array>, &last>(context->array_class, "last");
		method<Self<Array>, &empty>(context->array_class, "empty?");
		
		method<Self<Array>, Value, &equal>(context->array_class, "eql?");
		method<Self<Array>, Value, &equal>(context->array_class, "==");
		
		method<Self<Array>, &shift>(context->array_class, "shift");
		method<Self<Array>, Arg::Count, Arg::Values, &unshift>(context->array_class, "unshift");
		method<Self<Array>, Arg::Count, Arg::Values, &push>(context->array_class, "push");
		method<Self<Array>, Arg::Count, Arg::Values, &values_at>(context->array_class, "values_at");
		method<Self<Array>, Array, &add>(context->array_class, "+");
		method<Self<Array>, Array, &sub>(context->array_class, "-");
		method<Self<Array>, Arg::Count, Arg::Values, &push>(context->array_class, "<<");
		method<Self<Array>, &pop>(context->array_class, "pop");
		method<Self<Array>, &length>(context->array_class, "length");
		method<Self<Array>, &length>(context->array_class, "size");
		method<Self<Array>, &rb_flatten>(context->array_class, "flatten");
		method<Self<Array>, &rb_flatten_ex>(context->array_class, "flatten!");
		method<Self<Array>, Optional<String>, &join>(context->array_class, "join");
		method<Self<Array>, &to_s>(context->array_class, "to_s");
		method<Self<Array>, Arg::Block, &each>(context->array_class, "each");
		method<Self<Array>, Arg::Block, &reverse_each>(context->array_class, "reverse_each");
		method<Self<Array>, Value, Optional<Value>, &rb_get>(context->array_class, "[]");
		method<Self<Array>, Arg::UInt, Value, &rb_set>(context->array_class, "[]=");
	}
};

