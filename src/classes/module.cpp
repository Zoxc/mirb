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
	
	value_t Module::extend_object(value_t self, value_t obj)
	{
		OnStack<1> os(obj);

		include_module(auto_cast(singleton_class(obj)), auto_cast(self));

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
			if(type_error(argv[i], context->module_class))
				return 0;

			if(!call(argv[i], "append_features", 1, &obj))
				return 0;
			
			if(!call(argv[i], "included", 1, &obj))
				return 0;
		}

		return obj;
	}
	
	value_t attr_write(Frame &frame)
	{
		set_var(frame.obj, frame.code->symbol, frame.argv[0]);
		return frame.argv[0];
	}

	value_t attr_read(Frame &frame)
	{
		return get_var(frame.obj, frame.code->symbol);
	}

	void attr(value_t module, value_t sym, bool write)
	{
		auto self = cast<Module>(module);
		Symbol *name = cast<Symbol>(sym); 

		Block *result = Collector::allocate_pinned<Block>(nullptr);
		
		result->symbol = Symbol::get("@" + name->string);

		if(write)
		{
			name = Symbol::get(name->string + "=");
			result->executor = attr_write;
			result->min_args = 1;
			result->max_args = 1;
		}
		else
		{
			result->executor = attr_read;
			result->min_args = 0;
			result->max_args = 0;
		}
		
		Tuple<Module> *scope = Tuple<Module>::allocate(1);

		(*scope)[0] = self;

		self->set_method(name, Collector::allocate<Method>(result, scope));
	}
	
	value_t attr_setup(value_t obj, size_t argc, value_t argv[], bool read, bool write)
	{
		for(size_t i = 0; i < argc; ++i)
		{
			if(Value::of_type<Symbol>(argv[i]))
			{
				if(read)
					attr(obj, argv[i], false);
				if(write)
					attr(obj, argv[i], true);
			}
			else
				return raise(context->type_error, "Expected symbol");
		}

		return value_nil;
	}
	
	value_t attr_writer(value_t obj, size_t argc, value_t argv[])
	{
		return attr_setup(obj, argc, argv, false, true);
	}
	
	value_t attr_reader(value_t obj, size_t argc, value_t argv[])
	{
		return attr_setup(obj, argc, argv, true, false);
	}
	
	value_t attr_accessor(value_t obj, size_t argc, value_t argv[])
	{
		return attr_setup(obj, argc, argv, true, true);
	}
	
	value_t visibility_dummy(value_t obj, size_t, value_t[])
	{
		return obj;
	}
	
	value_t Module::alias_method(value_t obj, Symbol *new_name, Symbol *old_name)
	{
		auto self = cast<Module>(obj);

		auto method = self->get_method(auto_cast(old_name));

		if(prelude_unlikely(!method))
		{
			raise(context->name_error, "Unable to find method " + inspect_obj(old_name) + " on " + inspect_obj(obj));
			return value_raise;
		}

		self->set_method(auto_cast(new_name), method);

		return obj;
	}

	value_t Module::const_defined(Module *obj, Symbol *constant)
	{
		if(obj->vars)
			return auto_cast(obj->vars->map.has(constant));
		else
			return value_false;
	}

	value_t Module::const_get(Module *obj, Symbol *constant)
	{
		return get_vars(obj)->map.try_get(constant, [&]{
			return raise(context->name_error, "Uninitialized constant " + inspect_obj(obj) + "::" + constant->string);
		});
	}

	value_t Module::const_set(Module *obj, Symbol *constant, value_t value)
	{
		get_vars(obj)->map.set(constant, value);
		return value;
	}
	
	value_t Module::module_function(Module *obj, size_t argc, value_t argv[])
	{
		Class *meta = class_of(obj);

		OnStack<1> os(obj);

		for(size_t i = 0; i < argc; ++i)
		{
			if(type_error(argv[i], context->symbol_class))
				return 0;
			
			auto method = obj->get_method(auto_cast(argv[i]));

			if(prelude_unlikely(!method))
			{
				raise(context->name_error, "Unable to find method " + inspect_obj(argv[i]) + " on " + inspect_obj(obj));
				return value_raise;
			}

			meta->set_method(auto_cast(argv[i]), method);
		}

		return obj;
	}
	
	value_t Module::method_defined(Module *obj, Symbol *name)
	{
		return auto_cast(lookup_method(obj, name) != 0);
	}

	void Module::initialize()
	{
		method<Arg::Self>(context->module_class, "to_s", &to_s);
		method<Arg::Self, Arg::Class<Symbol>, Arg::Class<Symbol>>(context->module_class, "alias_method", &alias_method);
		method<Arg::Self, Arg::Class<Module>>(context->module_class, "append_features", &append_features);
		method<Arg::Self, Arg::Count, Arg::Values>(context->module_class, "include", &include);
		method<Arg::Value>(context->module_class, "included", &included);
		method<Arg::Value>(context->module_class, "extended", &included);
		method<Arg::Self, Arg::Value>(context->module_class, "extend_object", &extend_object);
		method<Arg::Self, Arg::Count, Arg::Values>(context->module_class, "attr_reader", &attr_reader);
		method<Arg::Self, Arg::Count, Arg::Values>(context->module_class, "attr_writer", &attr_writer);
		method<Arg::Self, Arg::Count, Arg::Values>(context->module_class, "attr_accessor", &attr_accessor);
		
		method<Arg::SelfClass<Module>, Arg::Count, Arg::Values>(context->module_class, "module_function", &module_function);
		
		method<Arg::Self, Arg::Count, Arg::Values>(context->module_class, "public", &Mirb::visibility_dummy);
		method<Arg::Self, Arg::Count, Arg::Values>(context->module_class, "private", &Mirb::visibility_dummy);
		method<Arg::Self, Arg::Count, Arg::Values>(context->module_class, "protected", &Mirb::visibility_dummy);
		
		method<Arg::SelfClass<Module>, Arg::Class<Symbol>>(context->module_class, "const_defined?", &const_defined);
		method<Arg::SelfClass<Module>, Arg::Class<Symbol>>(context->module_class, "const_get", &const_get);
		method<Arg::SelfClass<Module>, Arg::Class<Symbol>, Arg::Value>(context->module_class, "const_set", &const_set);

		method<Arg::SelfClass<Module>, Arg::Class<Symbol>>(context->module_class, "method_defined?", &method_defined);
	}
};

