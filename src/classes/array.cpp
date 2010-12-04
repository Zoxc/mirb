#include "array.hpp"
#include "string.hpp"
#include "fixnum.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Array::class_ref;

	mirb_compiled_block(Array_allocate)
	{
		return auto_cast(new (gc) Array(obj));
	}

	mirb_compiled_block(array_push)
	{
		auto self = cast<Array>(obj);

		MIRB_ARG_EACH(i)
		{
			self->vector.push(argv[i]);
		}

		return obj;
	}

	mirb_compiled_block(array_pop)
	{
		auto self = cast<Array>(obj);

		if(self->vector.size())
			return self->vector.pop();
		else
			return value_nil;
	}

	mirb_compiled_block(array_inspect)
	{
		auto self = cast<Array>(obj);

		CharArray result = "[";

		for(size_t i = 0; i < self->vector.size(); ++i)
		{
			String *desc = cast<String>(call(self->vector[i], "inspect", 0, 0));

			result += desc->string;

			if(i != self->vector.size() - 1)
				result += ", ";
		}

		result += "]";

		return result.to_string();
	}

	mirb_compiled_block(array_length)
	{
		auto self = cast<Array>(obj);

		return Fixnum::from_size_t(self->vector.size());
	}

	mirb_compiled_block(array_each)
	{
		auto self = cast<Array>(obj);

		for(auto i = self->vector.begin(); i != self->vector.end(); ++i)
			call(block, "call", 1, &(*i));

		return obj;
	}

	void Array::initialize()
	{
		Array::class_ref = define_class(Object::class_ref, "Array", Object::class_ref);
		
		define_singleton_method(Array::class_ref, "allocate", Array_allocate);
		
		define_method(Array::class_ref, "push", array_push);
		define_method(Array::class_ref, "<<", array_push);
		define_method(Array::class_ref, "pop", array_pop);
		define_method(Array::class_ref, "length", array_length);
		define_method(Array::class_ref, "inspect", array_inspect);
		define_method(Array::class_ref, "each", array_each);
	}
};

