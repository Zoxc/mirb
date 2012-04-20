#include "exception.hpp"
#include "symbol.hpp"
#include "../runtime.hpp"
#include "../collector.hpp"

namespace Mirb
{
	value_t Exception::class_ref;
	
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
		Exception::class_ref = define_class(Object::class_ref, "Exception", Object::class_ref);

		singleton_method<Arg::Self>(Exception::class_ref, "allocate", &allocate);

		static_method<Arg::Self, Arg::Value>(Exception::class_ref, "initialize", &method_initialize);
		static_method<Arg::Self>(Exception::class_ref, "message", &to_s);
		static_method<Arg::Self>(Exception::class_ref, "to_s", &to_s);
	}
};

