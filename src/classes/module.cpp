#include "class.hpp"
#include "symbol.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Module::class_ref;

	BlockMap *Module::get_methods()
	{
		if(!methods)
			methods = new BlockMap(methods_initial);
		
		return methods;
	}

	mirb_compiled_block(module_to_s)
	{
		value_t name = get_var(obj, Symbol::from_literal("__classname__"));

		if(Value::test(name))
			return name;

		return object_to_s(0, value_nil, obj, 0, 0, 0); // TODO: Replace by super
	}

	mirb_compiled_block(module_append_features)
	{
		include_module(MIRB_ARG(0), obj);

		return obj;
	}

	mirb_compiled_block(module_included)
	{
		return obj;
	}

	mirb_compiled_block(module_include)
	{
		MIRB_ARG_EACH(i)
		{
			call(argv[i], "append_features", 1, &obj);
			call(argv[i], "included", 1, &obj);
		}

		return obj;
	}
	
	void Module::initialize()
	{
		define_method(Module::class_ref, "to_s", module_to_s);
		define_method(Module::class_ref, "append_features", module_append_features);
		define_method(Module::class_ref, "include", module_include);
		define_method(Module::class_ref, "included", module_included);
	}
};

