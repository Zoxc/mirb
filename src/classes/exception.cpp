#include "exception.hpp"
#include "symbol.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Exception::class_ref;
	
	mirb_compiled_block(Exception_allocate)
	{
		return auto_cast(new (gc) Exception(obj));
	}

	mirb_compiled_block(exception_to_s)
	{
		auto self = cast<Exception>(obj);

		return self->message;
	}

	mirb_compiled_block(exception_initialize)
	{
		auto self = cast<Exception>(obj);

		self->message = MIRB_ARG(0);

		return obj;
	}

	void Exception::initialize()
	{
		Exception::class_ref = define_class(Object::class_ref, "Exception", Object::class_ref);

		define_singleton_method(Exception::class_ref, "allocate", Exception_allocate);

		define_method(Exception::class_ref, "initialize", exception_initialize);
		define_method(Exception::class_ref, "message", exception_to_s);
		define_method(Exception::class_ref, "to_s", exception_to_s);
	}
};

