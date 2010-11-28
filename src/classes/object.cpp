#include "object.hpp"
#include "class.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "../gc.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Object::class_ref;

	value_t Object::allocate(value_t instance_of)
	{
		return auto_cast(new (gc) Object(instance_of));
	}
	
	mirb_compiled_block(object_tap)
	{
		call(block, "call", 1, &obj);
		return obj;
	}

	mirb_compiled_block(Object_allocate)
	{
		return Object::allocate(obj);
	}

	mirb_compiled_block(object_inspect)
	{
		return call(obj, "to_s", 0, 0);
	}

	mirb_compiled_block(object_to_s)
	{
		value_t c = real_class_of(obj);
		value_t name = get_var(c, Symbol::from_literal("__classname__"));

		if(Value::test(name))
		{
			auto classname = cast<String>(get_var(c, Symbol::from_literal("__classname__")));

			CharArray result = "#<" + classname->string + ":0x" + CharArray::hex(obj) + ">";

			return result.to_string();
		}
		else
		{
			CharArray result = "#<0x" + CharArray::hex(obj) + ">";
			
			return result.to_string();
		}
	}

	mirb_compiled_block(object_dummy)
	{
		return value_nil;
	}

	mirb_compiled_block(object_equal)
	{
		return auto_cast(obj == MIRB_ARG(0));
	}

	mirb_compiled_block(object_not_equal)
	{
		return auto_cast(obj != MIRB_ARG(0));
	}

	mirb_compiled_block(object_not)
	{
		return auto_cast(!Value::test(obj));
	}
	
	void Object::initialize()
	{
		define_method(Object::class_ref, "initialize", object_dummy);
		define_method(Object::class_ref, "inspect", object_inspect);
		define_method(Object::class_ref, "to_s", object_to_s);
		define_method(Object::class_ref, "tap", object_tap);
		define_method(Object::class_ref, "equal?", object_equal);
		define_method(Object::class_ref, "eql?", object_equal);
		define_method(Object::class_ref, "==", object_equal);
		define_method(Object::class_ref, "===", object_equal);
		define_method(Object::class_ref, "!=", object_not_equal);
		define_method(Object::class_ref, "!", object_not);

		define_singleton_method(Object::class_ref, "allocate", Object_allocate);
	}
};

