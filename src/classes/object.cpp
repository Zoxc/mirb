#include "../runtime.hpp"
#include "../collector.hpp"
#include "../classes.hpp"

namespace Mirb
{
	void Object::generate_hash()
	{
		hash_value = (size_t)hash_number((size_t)this); // Note: The first two bits are constant
		this->hashed = true;
	}
	
	value_t Object::allocate(Class *instance_of)
	{
		return Collector::allocate<Object>(instance_of);
	}
	
	value_t Object::tap(value_t obj, value_t block)
	{
		OnStack<1> os(obj);

		if(!yield_argv(block, 1, &obj))
			return 0;

		return obj;
	}
	
	value_t Object::rb_inspect(value_t obj)
	{
		return call(obj, "to_s");
	}
	
	value_t Object::to_s(value_t obj)
	{
		value_t c = real_class_of(obj);
		value_t name = get_var(c, context->syms.classname);

		if(name != value_nil)
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
		return Value::from_bool(obj == other);
	}
	
	value_t Object::not_equal(value_t obj, value_t other)
	{
		return Value::from_bool(obj != other);
	}
	
	value_t Object::rb_not(value_t obj)
	{
		return Value::from_bool(!Value::test(obj));
	}

	value_t Object::instance_eval(value_t obj, String *string, value_t block)
	{
		if(!string)
		{
			Proc *proc = get_proc(block);

			if(!proc)
				return 0;
			else
				return Proc::call_with_options(obj, context->frame->prev->scope, proc, value_nil, 0, nullptr);
		}
		else
		{
			CharArray code = string->string.c_str();

			return eval(obj, Symbol::get("in eval"), context->frame->prev->scope, code.str_ref(), code.str_length(), "(eval)");
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
	
	value_t Object::kind_of(value_t obj, Class *klass)
	{
		return Value::from_bool(Mirb::kind_of(klass, obj));
	}
	
	value_t nil()
	{
		return value_false;
	}
	
	template<Value::Type type> struct DupObject
	{
		typedef value_t Result;
		typedef typename Value::TypeClass<type>::Class Class;

		static value_t func(value_t value)
		{
			return Class::dup(static_cast<Class *>(value));
		}
	};
	
	value_t Object::rb_dup(value_t obj)
	{
		value_t result = Value::virtual_do<DupObject>(Value::type(obj), obj);

		if(!result)
			return 0;

		OnStack<1> os(result);

		if(!call_argv(result, "initialize_copy", 1, &obj))
			return 0;

		return result;
	}
	
	value_t Object::initialize_copy(value_t)
	{
		return value_nil;
	}
	
	value_t instance_variable_set(value_t self, Symbol *name, value_t value)
	{
		if(name->string.size() == 0 || name->string[0] != '@')
			return raise(context->standard_error, "Invalid instance variable name");

		set_var(self, name, value);

		return value;
	}
	
	value_t instance_variable_get(value_t self, Symbol *name)
	{
		if(name->string.size() == 0 || name->string[0] != '@')
			return raise(context->standard_error, "Invalid instance variable name");

		return get_var(self, name);
	}
	
	void Object::initialize()
	{
		method<Arg::Self<Arg::Value>, Arg::Class<Symbol>, Arg::Value>(context->object_class, "instance_variable_set", &instance_variable_set);
		method<Arg::Self<Arg::Value>, Arg::Class<Symbol>>(context->object_class, "instance_variable_get", &instance_variable_get);

		method<Arg::Self<Arg::Value>>(context->object_class, "object_id", &object_id);
		method<Arg::Self<Arg::Value>>(context->object_class, "__id__", &object_id);
		method<Arg::Count>(context->object_class, "initialize", &variadic_dummy);
		context->inspect_method = method<Arg::Self<Arg::Value>>(context->object_class, "inspect", &rb_inspect);
		method(context->object_class, "nil?", &nil);
		method<Arg::Self<Arg::Value>>(context->object_class, "to_s", &to_s);
		method<Arg::Self<Arg::Value>>(context->object_class, "dup", &rb_dup);
		method<Arg::Self<Arg::Value>>(context->object_class, "class", &klass);
		method<Arg::Self<Arg::Value>>(context->object_class, "freeze", &dummy);
		method<Arg::Self<Arg::Value>, Arg::Class<Class>>(context->object_class, "is_a?", &kind_of);
		method<Arg::Self<Arg::Value>, Arg::Class<Class>>(context->object_class, "kind_of?", &kind_of);
		method<Arg::Self<Arg::Value>>(context->object_class, "frozen?", &dummy);
		method<Arg::Self<Arg::Value>, Arg::Value>(context->object_class, "=~", &pattern);
		method<Arg::Value>(context->object_class, "initialize_copy", &initialize_copy);
		method<Arg::Self<Arg::Value>, Arg::Count, Arg::Values>(context->object_class, "extend", &extend);
		method<Arg::Self<Arg::Value>, Arg::Block>(context->object_class, "tap", &tap);
		method<Arg::Self<Arg::Value>, Arg::Value>(context->object_class, "equal?", &equal);
		method<Arg::Self<Arg::Value>, Arg::Value>(context->object_class, "eql?", &equal);
		method<Arg::Self<Arg::Value>, Arg::Value>(context->object_class, "==", &equal);
		method<Arg::Self<Arg::Value>, Arg::Value>(context->object_class, "===", &equal);
		method<Arg::Self<Arg::Value>, Arg::Value>(context->object_class, "!=", &not_equal);
		method<Arg::Self<Arg::Value>>(context->object_class, "!", &rb_not);
		method<Arg::Self<Arg::Value>, Arg::Default<Arg::Class<String>>, Arg::Block>(context->object_class, "instance_eval", &instance_eval);

		singleton_method<Arg::Self<Arg::Class<Class>>>(context->object_class, "allocate", &allocate);
	}
};

