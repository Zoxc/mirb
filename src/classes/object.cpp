#include "../runtime.hpp"
#include "../internal.hpp"
#include "../classes.hpp"

namespace Mirb
{
	Object::Object(const Object &other) : Value(other), instance_of(other.instance_of), vars(nullptr), hash_value(other.hash_value)
	{
		if(other.vars)
		{
			vars = Collector::allocate<ValueMap>();

			Object *other_obj = (Object *)&other;

			ValueMapAccess::each_pair(other_obj->vars, [&](value_t key, value_t value) -> bool {
				ValueMapAccess::set(vars, key, value);
				return true;
			});
		}
	}

	void Object::generate_hash()
	{
		hash_value = (size_t)hash_number((size_t)this); // Note: The first two bits are constant
		this->hashed = true;
	}
	
	value_t Object::tap(value_t obj, value_t block)
	{
		OnStack<1> os(obj);

		yield(block, obj);

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
		value_t result = call(obj, "==", other);

		return Value::from_bool(!result->test());
	}
	
	value_t Object::not_match(value_t obj, value_t other)
	{
		value_t result = call(obj, "=~", other);

		return Value::from_bool(!result->test());
	}
	
	value_t Object::rb_not(value_t obj)
	{
		return Value::from_bool(!obj->test());
	}

	value_t Object::instance_eval(value_t obj, String *string, value_t block)
	{
		auto klass = singleton_class(obj);

		if(!string)
		{
			Proc *proc = get_proc(block);

			return call_code(proc->block, obj, proc->name, context->frame->scope->copy_and_prepend(klass), proc->scopes, block, 0, nullptr);
		}
		else
		{
			CharArray code = string->string.c_str();

			return eval(obj, Symbol::get("in eval"), context->frame->scope->copy_and_prepend(klass), code.str_ref(), code.str_length(), "(eval)");
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
			call(argv[i], "extend_object", obj);
			call(argv[i], "extended", obj);
		}

		return obj;
	}
	
	value_t object_id(value_t obj)
	{
		return Fixnum::from_size_t(hash_value(obj)); // TODO: Make a proper object_id
	}
	
	value_t Object::rb_kind_of(value_t obj, Class *klass)
	{
		return Value::from_bool(obj->kind_of(klass));
	}
	
	value_t Object::rb_instance_of(value_t obj, Class *klass)
	{
		return Value::from_bool(class_of(obj) == klass);
	}
	
	value_t Object::respond_to(value_t obj, Symbol *name)
	{
		return Value::from_bool(Mirb::respond_to(obj, name) != 0);
	}
	
	value_t nil()
	{
		return value_false;
	}
	
	template<Type::Enum type> struct DupObject
	{
		typedef value_t Result;
		typedef typename Type::ToClass<type>::Class Class;

		static value_t func(value_t value)
		{
			return Class::dup(static_cast<Class *>(value));
		}
	};
	
	value_t Object::rb_dup(value_t obj)
	{
		value_t result = Type::action<DupObject>(obj->type(), obj);

		OnStack<1> os(result);

		call(result, "initialize_copy", obj);

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
		internal_allocator<Object, &Context::object_class>();
		
		method<Self<Value>, Symbol, Value, &instance_variable_set>(context->object_class, "instance_variable_set");
		method<Self<Value>, Symbol, &instance_variable_get>(context->object_class, "instance_variable_get");

		method<Self<Value>, &object_id>(context->object_class, "object_id");
		method<Self<Value>, &object_id>(context->object_class, "__id__");
		method<Arg::Count, &variadic_dummy>(context->object_class, "initialize");
		context->inspect_method = method<Self<Value>, &rb_inspect>(context->object_class, "inspect");
		method<&nil>(context->object_class, "nil?");
		method<Self<Value>, &to_s>(context->object_class, "to_s");
		method<Self<Value>, &rb_dup>(context->object_class, "dup");
		method<Self<Value>, &rb_dup>(context->object_class, "clone");
		method<Self<Value>, &klass>(context->object_class, "class");
		method<&dummy>(context->object_class, "freeze");
		method<Self<Value>, Symbol, &respond_to>(context->object_class, "respond_to?");
		method<Self<Value>, Class, &rb_kind_of>(context->object_class, "is_a?");
		method<Self<Value>, Class, &rb_kind_of>(context->object_class, "kind_of?");
		method<Self<Value>, Class, &rb_instance_of>(context->object_class, "instance_of?");
		method<&dummy>(context->object_class, "frozen?");
		method<Value, &pattern>(context->object_class, "=~");
		method<Value, &initialize_copy>(context->object_class, "initialize_copy");
		method<Self<Value>, Arg::Count, Arg::Values, &extend>(context->object_class, "extend");
		method<Self<Value>, Arg::Block, &tap>(context->object_class, "tap");
		method<Self<Value>, Value, &equal>(context->object_class, "equal?");
		method<Self<Value>, Value, &equal>(context->object_class, "eql?");
		method<Self<Value>, Value, &equal>(context->object_class, "==");
		method<Self<Value>, Value, &equal>(context->object_class, "===");
		method<Self<Value>, Value, &not_equal>(context->object_class, "!=");
		method<Self<Value>, Value, &not_match>(context->object_class, "!~");
		method<Self<Value>, &rb_not>(context->object_class, "!");
		method<Self<Value>, Optional<String>, Arg::Block, &instance_eval>(context->object_class, "instance_eval");
		method<Self<Value>, Symbol, Arg::Count, Arg::Values, &rb_send>(context->object_class, "send");
		method<Self<Value>, Symbol, Arg::Count, Arg::Values, &rb_send>(context->object_class, "__send__");
	}
};

