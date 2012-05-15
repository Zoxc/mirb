#include "object.hpp"
#include "class.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "../runtime.hpp"
#include "../collector.hpp"
#include "proc.hpp"
#include "fixnum.hpp"

namespace Mirb
{
	void Object::generate_hash()
	{
		hash_value = hash_number((size_t)this); // Note: The first two bits are constant
		this->hashed = true;
	}
	
	value_t Object::allocate(Class *instance_of)
	{
		return auto_cast(Collector::allocate<Object>(instance_of));
	}
	
	value_t Object::tap(value_t obj, value_t block)
	{
		OnStack<1> os(obj);

		if(yield_argv(block, 1, &obj) == value_raise)
			return value_raise;

		return obj;
	}
	
	value_t Object::inspect(value_t obj)
	{
		return call(obj, "to_s");
	}
	
	value_t Object::to_s(value_t obj)
	{
		value_t c = real_class_of(obj);
		value_t name = get_var(c, context->syms.classname);

		if(Value::test(name))
		{
			auto classname = cast<String>(name);

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
	
	value_t Object::variadic_dummy(size_t)
	{
		return value_nil;
	}
	
	value_t Object::pattern(value_t)
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

	value_t Object::instance_eval(value_t obj, size_t argc, value_t argv[], value_t block)
	{
		if(argc > 1)
			return raise(context->argument_error, "Too many arguments.");

		if(argc == 0)
		{
			if(Value::type(block) == Value::Proc)
				return Proc::call_with_options(obj, current_frame->prev->scope, block, value_nil, 0, nullptr);
			else
				return raise(context->type_error, "Expected block to evaluate");
		}
		else
		{
			if(Value::type(argv[0]) == Value::String)
			{
				CharArray code = cast<String>(argv[0])->string.c_str();

				return eval(obj, Symbol::get("in eval"), current_frame->prev->scope, code.str_ref(), code.str_length(), "(eval)");
			}
			else
				return raise(context->type_error, "Expected string");
		}
	}
	
	value_t Object::klass(value_t obj)
	{
		return real_class_of(obj);
	}
	
	value_t Object::extend(value_t obj, size_t argc, value_t argv[])
	{
		OnStack<1> os(obj);

		for(size_t i = 0; i < argc; ++i)
		{
			if(type_error(argv[i], context->module_class))
				return 0;

			if(!call_argv(argv[i], "extend_object", 1, &obj))
				return 0;
			
			if(!call_argv(argv[i], "extended", 1, &obj))
				return 0;
		}

		return obj;
	}
	
	value_t object_id(value_t obj)
	{
		return Fixnum::from_size_t(Value::hash(obj)); // TODO: Make a proper object_id
	}
	
	value_t kind_of(value_t obj, Class *klass)
	{
		Class *c = class_of(obj);

		while(c)
		{
			c = real_class(c);

			if(!c)
				break;

			if(c == klass)
				return value_true;

			c = c->superclass;
		}

		return value_false;
	}
	
	value_t nil()
	{
		return value_false;
	}
	
	void Object::initialize()
	{
		method<Arg::Self>(context->object_class, "object_id", &object_id);
		method<Arg::Self>(context->object_class, "__id__", &object_id);
		method<Arg::Count>(context->object_class, "initialize", &variadic_dummy);
		context->inspect_method = method<Arg::Self>(context->object_class, "inspect", &inspect);
		method(context->object_class, "nil?", &nil);
		method<Arg::Self>(context->object_class, "to_s", &to_s);
		method<Arg::Self>(context->object_class, "class", &klass);
		method<Arg::Self>(context->object_class, "freeze", &dummy);
		method<Arg::Self, Arg::Class<Class>>(context->object_class, "is_a?", &kind_of);
		method<Arg::Self, Arg::Class<Class>>(context->object_class, "kind_of?", &kind_of);
		method<Arg::Self>(context->object_class, "frozen?", &dummy);
		method<Arg::Self, Arg::Value>(context->object_class, "=~", &pattern);
		method<Arg::Self, Arg::Count, Arg::Values>(context->object_class, "extend", &extend);
		method<Arg::Self, Arg::Block>(context->object_class, "tap", &tap);
		method<Arg::Self, Arg::Value>(context->object_class, "equal?", &equal);
		method<Arg::Self, Arg::Value>(context->object_class, "eql?", &equal);
		method<Arg::Self, Arg::Value>(context->object_class, "==", &equal);
		method<Arg::Self, Arg::Value>(context->object_class, "===", &equal);
		method<Arg::Self, Arg::Value>(context->object_class, "!=", &not_equal);
		method<Arg::Self>(context->object_class, "!", &method_not);
		method<Arg::Self, Arg::Count, Arg::Values, Arg::Block>(context->object_class, "instance_eval", &instance_eval);

		singleton_method<Arg::SelfClass<Class>>(context->object_class, "allocate", &allocate);
	}
};

