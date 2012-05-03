#include "class.hpp"
#include "symbol.hpp"
#include "../runtime.hpp"
#include "../collector.hpp"

namespace Mirb
{
	ValueMap *Module::get_methods()
	{
		if(prelude_unlikely(!methods))
			methods = Collector::allocate<ValueMap>();
		
		return methods;
	}
	
	Method *Module::get_method(Symbol *name)
	{
		return auto_cast_null(get_methods()->map.get(name));
	}

	void Module::set_method(Symbol *name, Method *method)
	{
		Value::assert_valid(method);

		return get_methods()->map.set(name, auto_cast_null(method));
	}

	value_t Module::to_s(value_t obj)
	{
		value_t name = get_var(obj, Symbol::from_literal("__classname__"));

		if(Value::test(name))
			return name;

		return Object::to_s(obj); // TODO: Replace by super
	}
	
	value_t Module::append_features(value_t obj, value_t mod)
	{
		OnStack<1> os(obj);

		include_module(auto_cast(mod), auto_cast(obj));

		return obj;
	}
	
	value_t Module::included(value_t obj)
	{
		return obj;
	}
	
	value_t Module::include(value_t obj, size_t argc, value_t argv[])
	{
		OnStack<1> os(obj);

		for(size_t i = 0; i < argc; ++i)
		{
			if(type_error(argv[i],  context->module_class))
				return 0;

			if(!call(argv[i], "append_features", 1, &obj))
				return 0;
			
			if(!call(argv[i], "included", 1, &obj))
				return 0;
		}

		return obj;
	}
	
	void Module::initialize()
	{
		method<Arg::Self>(context->module_class, "to_s", &to_s);
		method<Arg::Self, Arg::Class<Module>>(context->module_class, "append_features", &append_features);
		method<Arg::Self, Arg::Count, Arg::Values>(context->module_class, "include", &include);
		method<Arg::Class<Module>>(context->module_class, "included", &included);
	}
};

