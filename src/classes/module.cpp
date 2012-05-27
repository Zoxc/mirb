#include "class.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "../runtime.hpp"
#include "../collector.hpp"
#include "../value-map.hpp"

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
		return cast_null<Method>(ValueMapAccess::get(get_methods(), name, [] { return nullptr; }));
	}

	void Module::set_method(Symbol *name, Method *method)
	{
		Value::assert_valid(method);

		ValueMapAccess::set(get_methods(), name, method);
	}

	value_t Module::to_s(value_t obj)
	{
		value_t name = get_var(obj, context->syms.classname);

		if(Value::test(name))
			return name;

		return Object::to_s(obj); // TODO: Replace by super
	}
	
	value_t Module::append_features(Module *obj, Module *mod)
	{
		OnStack<1> os(obj);

		include_module(mod, obj);

		return obj;
	}
	
	value_t Module::extend_object(Module *self, value_t obj)
	{
		OnStack<1> os(obj);

		include_module(singleton_class(obj), self);

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
			call_argv(argv[i], "append_features", 1, &obj);
			call_argv(argv[i], "included", 1, &obj);
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

	void attr(Module *self, Symbol *name, bool write)
	{
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
	
	value_t attr_setup(Module *obj, size_t argc, value_t argv[], bool read, bool write)
	{
		for(size_t i = 0; i < argc; ++i)
		{
			auto symbol = raise_cast<Symbol>(argv[i]);

			if(read)
				attr(obj, symbol, false);
			if(write)
				attr(obj, symbol, true);
		}

		return value_nil;
	}
	
	value_t attr_writer(Module *obj, size_t argc, value_t argv[])
	{
		return attr_setup(obj, argc, argv, false, true);
	}
	
	value_t attr_reader(Module *obj, size_t argc, value_t argv[])
	{
		return attr_setup(obj, argc, argv, true, false);
	}
	
	value_t attr_accessor(Module *obj, size_t argc, value_t argv[])
	{
		return attr_setup(obj, argc, argv, true, true);
	}
	
	value_t visibility_dummy(value_t obj, size_t, value_t[])
	{
		return obj;
	}
	
	value_t Module::alias_method(Module *self, Symbol *new_name, Symbol *old_name)
	{
		auto method = lookup_method(self, old_name);

		if(prelude_unlikely(!method))
		{
			OnStack<1> os(old_name);

			CharArray obj_str = pretty_inspect(self);

			raise(context->name_error, "Undefined method '" + old_name->string + "' for " + obj_str);

			return value_raise;
		}

		self->set_method(new_name, method);

		return self;
	}

	value_t Module::const_defined(Module *obj, Symbol *constant)
	{
		if(obj->vars)
			return ValueMapAccess::has(obj->vars, constant);
		else
			return value_false;
	}

	value_t Module::const_get(Module *obj, Symbol *constant)
	{
		return ValueMapAccess::get(get_vars(obj), constant, [&]() -> value_t {
			OnStack<1> os(constant);

			CharArray obj_str = inspect(obj);

			raise(context->name_error, "Uninitialized constant " + obj_str + "::" + constant->string);
		});
	}

	value_t Module::const_set(Module *obj, Symbol *constant, value_t value)
	{
		ValueMapAccess::set(get_vars(obj), constant, value);
		return value;
	}
	
	value_t Module::module_function(Module *obj, size_t argc, value_t argv[])
	{
		Class *meta = obj->instance_of;

		for(size_t i = 0; i < argc; ++i)
		{
			auto symbol = raise_cast<Symbol>(argv[i]);

			auto method = obj->get_method(symbol);

			if(prelude_unlikely(!method))
			{
				OnStack<1> os2(symbol);

				CharArray obj_value = pretty_inspect(obj);

				raise(context->name_error, "Unable to find method " + symbol->string + " on " + obj_value);
				return value_raise;
			}

			meta->set_method(symbol, method);
		}

		return obj;
	}
	
	value_t Module::method_defined(Module *obj, Symbol *name)
	{
		return Value::from_bool(lookup_method(obj, name) != 0);
	}

	void Module::initialize()
	{
		method<Arg::Self<Arg::Value>, &to_s>(context->module_class, "to_s");
		method<Arg::Self<Arg::Class<Module>>, Arg::Class<Symbol>, Arg::Class<Symbol>, &alias_method>(context->module_class, "alias_method");
		method<Arg::Self<Arg::Class<Module>>, Arg::Class<Module>, &append_features>(context->module_class, "append_features");
		method<Arg::Self<Arg::Value>, Arg::Count, Arg::Values, &include>(context->module_class, "include");
		method<Arg::Value, &included>(context->module_class, "included");
		method<Arg::Value, &included>(context->module_class, "extended");
		method<Arg::Self<Arg::Class<Module>>, Arg::Value, &extend_object>(context->module_class, "extend_object");
		method<Arg::Self<Arg::Class<Module>>, Arg::Count, Arg::Values, &attr_reader>(context->module_class, "attr_reader");
		method<Arg::Self<Arg::Class<Module>>, Arg::Count, Arg::Values, &attr_writer>(context->module_class, "attr_writer");
		method<Arg::Self<Arg::Class<Module>>, Arg::Count, Arg::Values, &attr_accessor>(context->module_class, "attr_accessor");
		
		method<Arg::Self<Arg::Class<Module>>, Arg::Count, Arg::Values, &module_function>(context->module_class, "module_function");
		
		method<Arg::Self<Arg::Value>, Arg::Count, Arg::Values, &Mirb::visibility_dummy>(context->module_class, "public");
		method<Arg::Self<Arg::Value>, Arg::Count, Arg::Values, &Mirb::visibility_dummy>(context->module_class, "private");
		method<Arg::Self<Arg::Value>, Arg::Count, Arg::Values, &Mirb::visibility_dummy>(context->module_class, "protected");
		
		method<Arg::Self<Arg::Class<Module>>, Arg::Class<Symbol>, &const_defined>(context->module_class, "const_defined?");
		method<Arg::Self<Arg::Class<Module>>, Arg::Class<Symbol>, &const_get>(context->module_class, "const_get");
		method<Arg::Self<Arg::Class<Module>>, Arg::Class<Symbol>, Arg::Value, &const_set>(context->module_class, "const_set");

		method<Arg::Self<Arg::Class<Module>>, Arg::Class<Symbol>, &method_defined>(context->module_class, "method_defined?");
	}
};

