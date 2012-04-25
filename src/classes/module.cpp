#include "class.hpp"
#include "symbol.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Module::class_ref;

	ValueMap *Module::get_methods()
	{
		if(prelude_unlikely(!methods))
			methods = Collector::allocate<ValueMap>();
		
		return methods;
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

		include_module(mod, obj);

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
			if(type_error(argv[i],  Module::class_ref))
				return value_raise;

			if(call(argv[i], "append_features", 1, &obj) == value_raise)
				return value_raise;
			
			if(call(argv[i], "included", 1, &obj) == value_raise)
				return value_raise;
		}

		return obj;
	}
	
	void Module::initialize()
	{
		static_method<Arg::Self>(Module::class_ref, "to_s", &to_s);
		static_method<Arg::Self, Arg::Class<Module>>(Module::class_ref, "append_features", &append_features);
		static_method<Arg::Self, Arg::Count, Arg::Values>(Module::class_ref, "include", &include);
		static_method<Arg::Class<Module>>(Module::class_ref, "included", &included);
	}
};

