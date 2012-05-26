#include "float.hpp"
#include "../runtime.hpp"
#include "../platform/platform.hpp"
#include "string.hpp"
#include "fixnum.hpp"

namespace Mirb
{
	value_t Float::to_s(Float *obj)
	{
		char_t buffer[32];

		size_t length = sprintf((char *)buffer, "%f", obj->value);

		return CharArray(buffer, length).to_string();
	}
	
	value_t Float::zero(Float *obj)
	{
		return Value::from_bool(obj->value == 0.0);
	}
	
	value_t Float::add(Float *obj, Float *other)
	{
		return Collector::allocate<Float>(obj->value + other->value);
	}
	
	value_t Float::sub(Float *obj, Float *other)
	{
		return Collector::allocate<Float>(obj->value - other->value);
	}
	
	value_t Float::mul(Float *obj, Float *other)
	{
		return Collector::allocate<Float>(obj->value * other->value);
	}
	
	value_t Float::div(Float *obj, Float *other)
	{
		return Collector::allocate<Float>(obj->value / other->value);
	}

	value_t Float::compare(Float *obj, Float *other)
	{
		return Fixnum::from_int(obj->value == other->value ? 0 : (obj->value > other->value ? 1 : -1));
	}

	void Float::initialize()
	{
		context->float_class = define_class("Float", context->numeric_class);
		
		method<Arg::Self<Arg::Class<Float>>>(context->float_class, "to_s", &to_s);
		method<Arg::Self<Arg::Class<Float>>>(context->float_class, "zero?", &zero);

		method<Arg::Self<Arg::Class<Float>>, Arg::Class<Float>>(context->float_class, "+", &add);
		method<Arg::Self<Arg::Class<Float>>, Arg::Class<Float>>(context->float_class, "-", &sub);
		method<Arg::Self<Arg::Class<Float>>, Arg::Class<Float>>(context->float_class, "*", &mul);
		method<Arg::Self<Arg::Class<Float>>, Arg::Class<Float>>(context->float_class, "/", &div);
		method<Arg::Self<Arg::Class<Float>>, Arg::Class<Float>>(context->float_class, "<=>", &compare);
	}
};

