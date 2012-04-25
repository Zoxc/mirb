#include "object.hpp"
#include "class.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Object::class_ref;
	Block *Object::inspect_block;

	value_t Object::allocate(value_t instance_of)
	{
		return auto_cast(Collector::allocate<Object>(instance_of));
	}
	
	value_t Object::tap(value_t obj, value_t block)
	{
		OnStack<1> os(obj);

		if(yield(block, 1, &obj) == value_raise)
			return value_raise;

		return obj;
	}
	
	value_t Object::inspect(value_t obj)
	{
		return call(obj, "to_s", 0, 0);
	}
	
	value_t Object::to_s(value_t obj)
	{
		value_t c = real_class_of(obj);
		value_t name = get_var(c, Symbol::from_literal("__classname__"));

		if(Value::test(name))
		{
			auto classname = cast<String>(get_var(c, Symbol::from_literal("__classname__")));

			CharArray result = "#<" + classname->string + ":0x" + CharArray::hex((size_t)obj) + ">";

			return result.to_string();
		}
		else
		{
			CharArray result = "#<0x" + CharArray::hex((size_t)obj) + ">";
			
			return result.to_string();
		}
	}
	
	value_t Object::dummy()
	{
		return value_nil;
	}
	
	value_t Object::equal(value_t obj, value_t other)
	{
		return auto_cast(obj == other);
	}
	
	value_t Object::not_equal(value_t obj, value_t other)
	{
		return auto_cast(obj != other);
	}
	
	value_t Object::method_not(value_t obj)
	{
		return auto_cast(!Value::test(obj));
	}
	
	void Object::initialize()
	{
		static_method(Object::class_ref, "initialize", &dummy);
		inspect_block = static_method<Arg::Self>(Object::class_ref, "inspect", &inspect);
		static_method<Arg::Self>(Object::class_ref, "to_s", &to_s);
		static_method<Arg::Self, Arg::Block>(Object::class_ref, "tap", &tap);
		static_method<Arg::Self, Arg::Value>(Object::class_ref, "equal?", &equal);
		static_method<Arg::Self, Arg::Value>(Object::class_ref, "eql?", &equal);
		static_method<Arg::Self, Arg::Value>(Object::class_ref, "==", &equal);
		static_method<Arg::Self, Arg::Value>(Object::class_ref, "===", &equal);
		static_method<Arg::Self, Arg::Value>(Object::class_ref, "!=", &not_equal);
		static_method<Arg::Self>(Object::class_ref, "!", &method_not);

		singleton_method<Arg::Self>(Object::class_ref, "allocate", &allocate);
	}
};

