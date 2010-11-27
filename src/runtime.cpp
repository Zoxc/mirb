#include "runtime.hpp"
#include "classes/object.hpp"
#include "classes/symbol.hpp"
#include "classes/class.hpp"

namespace Mirb
{
	std::string inspect(value_t obj)
	{
		return "unknown"; // TODO: Fix this
	}

	ValueMap *get_vars(value_t obj)
	{
		if(!Value::of_type<Object>(obj))
			Mirb::runtime_fail("No value map on objects passed by value"); // TODO: Fix this

		Object *object = auto_cast(obj);

		if(!object->vars)
			object->vars = new ValueMap(Object::vars_initial);
		
		return object->vars;
	}
	
	value_t get_const(value_t obj, Symbol *name)
	{
		// TODO: Fix constant lookup

		auto lookup = [&](value_t obj) -> value_t {
			while(obj)
			{
				ValueMap *vars = get_vars(obj);

				value_t value = vars->get(auto_cast(name));

				if(value != value_undef)
					return value;

				obj = cast<Class>(obj)->superclass;
			}

			return value_undef;
		};

		value_t result = lookup(obj);
		if(result != value_undef)
			return result;
			
		result = lookup(Object::class_ref);
		if(result != value_undef)
			return result;

		Mirb::debug_fail("Unable to find constant " + name->get_string() + " on " + inspect(obj) + "\n");
	}

	void set_const(value_t obj, Symbol *name, value_t value)
	{
		ValueMap *vars = get_vars(obj);

		value_t *old = vars->get_ref(auto_cast(name));

		if (old)
			set_var(obj, name, *old);
		else
		{
			std::cout << "Warning: Reassigning constant " << name->get_string() << " to " << inspect(value) << "\n";
			*old = value;
		}
	}
	
	value_t get_ivar(value_t obj, Symbol *name)
	{
		return get_var(obj, name);
	}

	void set_ivar(value_t obj, Symbol *name, value_t value)
	{
		set_var(obj, name, value);
	}

	value_t get_var(value_t obj, Symbol *name)
	{
		return get_vars(obj)->try_get(auto_cast(name), []{ return value_nil; });
	}

	void set_var(value_t obj, Symbol *name, value_t value)
	{
		get_vars(obj)->set(auto_cast(name), value);
	}
	
	Block *get_method(value_t obj, Symbol *name)
	{
		return cast<Class>(obj)->get_methods()->get(name);
	}

	void set_method(value_t obj, Symbol *name, Block *method)
	{
		return cast<Class>(obj)->get_methods()->set(name, method);
	}
};

