#include "class.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	Class *Class::create_initial(Class *superclass)
	{
		Class *result = Collector::setup_object<Class>(new (Collector::allocate_object<Class>()) Class(Value::Class));

		result->superclass = superclass;

		return result;
	}

	Class *Class::create_include_class(Module *module, Class *superclass)
	{
		if(Value::type(module) == Value::IClass)
			module = module->original_module;

		Class *result = Collector::setup_object<Class>(new (Collector::allocate_object<Class>()) Class(Value::IClass));

		result->superclass = superclass;
		result->original_module = module;
		result->methods = module->get_methods();
		result->vars = get_vars(module);

		return result;
	}

	value_t Class::to_s(Class *self)
	{
		value_t name = get_var(self, context->syms.classname);
		
		if(Value::test(name))
			return name;
		else if(self->singleton)
		{
			value_t real = get_var(self, context->syms.attached);

			auto real_str = inspect(real);

			CharArray result = "#<Class:" + real_str + ">";

			return result.to_string();
		}
		else
		{
			CharArray result = "#<Class:0x" + CharArray::hex((size_t)self) + ">";

			return result.to_string();
		}
	}
	
	value_t Class::rb_superclass(Module *obj)
	{
		value_t super = real_class(obj->superclass);

		if(super)
			return super;
		else
			return value_nil;
	}
	
	value_t Class::case_equal(Class *obj, value_t other)
	{
		return Value::from_bool(Mirb::kind_of(obj, other));
	}

	value_t Class::rb_new(value_t obj, size_t argc, value_t argv[])
	{
		value_t result = call(obj, "allocate");

		OnStack<1> os(result);

		call_argv(result, "initialize", argc, argv);

		return result;
	}

	void Class::initialize()
	{
		method<Arg::Self<Arg::Class<Class>>, Arg::Value, &case_equal>(context->class_class, "===");
		method<Arg::Self<Arg::Class<Class>>, &to_s>(context->class_class, "to_s");
		method<Arg::Self<Arg::Class<Module>>, &rb_superclass>(context->class_class, "superclass");
		method<Arg::Self<Arg::Value>, Arg::Count, Arg::Values, &rb_new>(context->class_class, "new");
	}
};
