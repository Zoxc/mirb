#include "exception.hpp"
#include "symbol.hpp"
#include "../runtime.hpp"
#include "../collector.hpp"

namespace Mirb
{
	value_t Exception::allocate(value_t obj)
	{
		return auto_cast(Collector::allocate<Exception>(Value::Exception, obj, value_nil, value_nil));
	}

	value_t Exception::to_s(value_t obj)
	{
		auto self = cast<Exception>(obj);

		return self->message;
	}

	value_t Exception::method_initialize(value_t obj, value_t message)
	{
		auto self = cast<Exception>(obj);

		self->message = message;

		return obj;
	}

	void Exception::initialize()
	{
		context->exception_class = define_class(context->object_class, "Exception", context->object_class);

		singleton_method<Arg::Self>(context->exception_class, "allocate", &allocate);

		static_method<Arg::Self, Arg::Value>(context->exception_class, "initialize", &method_initialize);
		static_method<Arg::Self>(context->exception_class, "message", &to_s);
		static_method<Arg::Self>(context->exception_class, "to_s", &to_s);
	}
};

