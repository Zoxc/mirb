#include "class.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	Class::Class(value_t module, value_t superclass) : Module(Value::IClass, module, superclass), singleton(false)
	{
		methods = cast<Module>(module)->get_methods();
		vars = get_vars(module);
	}

	value_t Class::to_s(value_t obj)
	{
		auto self = cast<Class>(obj);

		value_t name = get_var(obj, Symbol::from_literal("__classname__"));
		
		if(Value::test(name))
			return name;
		else if(self->singleton)
		{
			value_t real = get_var(obj, Symbol::from_literal("__attached__"));

			OnStack<1> os(real);

			real = call(real, "inspect");

			String *real_string = cast<String>(real);

			CharArray result = "#<Class:" + real_string->string + ">";

			return result.to_string();
		}
		else
		{
			CharArray result = "#<Class:0x" + CharArray::hex((size_t)obj) + ">";

			return result.to_string();
		}
	}
	
	value_t Class::method_superclass(value_t obj)
	{
		value_t super = real_class(cast<Module>(obj)->superclass);

		if(super)
			return super;
		else
			return value_nil;
	}

	value_t Class::method_new(value_t obj, size_t argc, value_t argv[])
	{
		value_t result = call(obj, "allocate", 0, 0);

		OnStack<1> os(result);

		call(result, "initialize", argc, argv);

		return result;
	}

	void Class::initialize()
	{
		static_method<Arg::Self>(context->class_class, "to_s", &to_s);
		static_method<Arg::Self>(context->class_class, "superclass", &method_superclass);
		static_method<Arg::Self, Arg::Count, Arg::Values>(context->class_class, "new", &method_new);
	}
};
