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

		yield_argv(block, 1, &obj);

		return obj;
	}
	
	value_t Object::rb_inspect(value_t obj)
	{
		return call(obj, "to_s");
	}
	
	value_t Object::to_s(value_t obj)
	{
		value_t c = class_of(obj);
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
		value_t result = call_argv(obj, "==", 1, &other);

		return Value::from_bool(!Value::test(result));
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
		return class_of(obj);
	}
	
	value_t Object::extend(value_t obj, size_t argc, value_t argv[])
	{
		OnStack<1> os(obj);

		for(size_t i = 0; i < argc; ++i)
		{
			call_argv(argv[i], "extend_object", 1, &obj);
			call_argv(argv[i], "extended", 1, &obj);
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
	
	value_t Object::respond_to(value_t obj, Symbol *name)
	{
		return Value::from_bool(Mirb::respond_to(obj, name) != 0);
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

		OnStack<1> os(result);

		call_argv(result, "initialize_copy", 1, &obj);

		return result;
	}
	
	value_t Object::initialize_copy(value_t)
	{
		return value_nil;
	}
	
	value_t instance_variable_set(value_t self, Symbol *name, value_t value)
	{
		if(name->string.size() == 0 || name->string[0] != '@')
			raise(context->standard_error, "Invalid instance variable name");

		set_var(self, name, value);

		return value;
	}
	
	value_t instance_variable_get(value_t self, Symbol *name)
	{
		if(name->string.size() == 0 || name->string[0] != '@')
			raise(context->standard_error, "Invalid instance variable name");

		return get_var(self, name);
	}
	
	value_t rb_send(value_t self, Symbol *name, size_t argc, value_t argv[])
	{
		return call_argv(self, name, argc, argv);
	}
	
	void Object::initialize()
	{
		method<Arg::Self<Arg::Value>, Arg::Class<Symbol>, Arg::Value, &instance_variable_set>(context->object_class, "instance_variable_set");
		method<Arg::Self<Arg::Value>, Arg::Class<Symbol>, &instance_variable_get>(context->object_class, "instance_variable_get");

		method<Arg::Self<Arg::Value>, &object_id>(context->object_class, "object_id");
		method<Arg::Self<Arg::Value>, &object_id>(context->object_class, "__id__");
		method<Arg::Count, &variadic_dummy>(context->object_class, "initialize");
		context->inspect_method = method<Arg::Self<Arg::Value>, &rb_inspect>(context->object_class, "inspect");
		method<&nil>(context->object_class, "nil?");
		method<Arg::Self<Arg::Value>, &to_s>(context->object_class, "to_s");
		method<Arg::Self<Arg::Value>, &rb_dup>(context->object_class, "dup");
		method<Arg::Self<Arg::Value>, &klass>(context->object_class, "class");
		method<&dummy>(context->object_class, "freeze");
		method<Arg::Self<Arg::Value>, Arg::Class<Symbol>, &respond_to>(context->object_class, "respond_to?");
		method<Arg::Self<Arg::Value>, Arg::Class<Class>, &kind_of>(context->object_class, "is_a?");
		method<Arg::Self<Arg::Value>, Arg::Class<Class>, &kind_of>(context->object_class, "kind_of?");
		method<&dummy>(context->object_class, "frozen?");
		method<Arg::Value, &pattern>(context->object_class, "=~");
		method<Arg::Value, &initialize_copy>(context->object_class, "initialize_copy");
		method<Arg::Self<Arg::Value>, Arg::Count, Arg::Values, &extend>(context->object_class, "extend");
		method<Arg::Self<Arg::Value>, Arg::Block, &tap>(context->object_class, "tap");
		method<Arg::Self<Arg::Value>, Arg::Value, &equal>(context->object_class, "equal?");
		method<Arg::Self<Arg::Value>, Arg::Value, &equal>(context->object_class, "eql?");
		method<Arg::Self<Arg::Value>, Arg::Value, &equal>(context->object_class, "==");
		method<Arg::Self<Arg::Value>, Arg::Value, &equal>(context->object_class, "===");
		method<Arg::Self<Arg::Value>, Arg::Value, &not_equal>(context->object_class, "!=");
		method<Arg::Self<Arg::Value>, &rb_not>(context->object_class, "!");
		method<Arg::Self<Arg::Value>, Arg::Optional<Arg::Class<String>>, Arg::Block, &instance_eval>(context->object_class, "instance_eval");
		method<Arg::Self<Arg::Value>, Arg::Class<Symbol>, Arg::Count, Arg::Values, &rb_send>(context->object_class, "send");
		method<Arg::Self<Arg::Value>, Arg::Class<Symbol>, Arg::Count, Arg::Values, &rb_send>(context->object_class, "__send__");

		singleton_method<Arg::Self<Arg::Class<Class>>, &allocate>(context->object_class, "allocate");
	}
};

