#include "class.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Class::class_ref;
	
	mirb_compiled_block(class_to_s)
	{
		auto self = cast<Class>(obj);

		value_t name = get_var(obj, Symbol::from_literal("__classname__"));
		
		if(Value::test(name))
			return name;
		else if(self->singleton)
		{
			value_t real = get_var(obj, Symbol::from_literal("__attached__"));
			String *real_string = cast<String>(real);

			real = call(real, "inspect", 0, 0);

			CharArray result = "#<Class:" + real_string->string + ">";

			return result.to_string();
		}
		else
		{
			CharArray result = "#<Class:0x" + CharArray::hex(obj) + ">";

			return result.to_string();
		}
	}

	mirb_compiled_block(class_superclass)
	{
		rt_value super = real_class(cast<Module>(obj)->superclass);

		if(super)
			return super;
		else
			return value_nil;
	}

	mirb_compiled_block(class_new)
	{
		value_t result = call(obj, "allocate", 0, 0);

		call(result, "initialize", argc, argv);

		return result;
	}

	void Class::initialize()
	{
		define_method(Class::class_ref, "to_s", class_to_s);
		define_method(Class::class_ref, "superclass", class_superclass);
		define_method(Class::class_ref, "new", class_new);
	}
};
