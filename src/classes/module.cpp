#include "class.hpp"
#include "symbol.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Module::class_ref;

	BlockMap *Module::get_methods()
	{
		if(!methods)
			methods = new BlockMap(methods_initial); // TODO: Allocate with GC
		
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

		MIRB_ARG_EACH(i)
		{
			call(argv[i], "append_features", 1, &obj);
			call(argv[i], "included", 1, &obj);
		}

		return obj;
	}
	
	void Module::initialize()
	{
		static_method<Arg::Self>(Module::class_ref, "to_s", &to_s);
		static_method<Arg::Self, Arg::Value>(Module::class_ref, "append_features", &append_features);
		static_method<Arg::Self, Arg::Count, Arg::Values>(Module::class_ref, "include", &include);
		static_method<Arg::Self>(Module::class_ref, "included", &included);
	}
};

