#include "class.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "proc.hpp"
#include "array.hpp"
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
	
	value_t Module::get_method(Symbol *name)
	{
		return ValueMapAccess::get(get_methods(), name, [] { return nullptr; });
	}

	void Module::set_method(Symbol *name, value_t method)
	{
		method->assert_valid();

		ValueMapAccess::set(get_methods(), name, method);
	}

	value_t Module::to_s(value_t obj)
	{
		value_t name = get_var(obj, context->syms.classpath);

		if(name->test())
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
			call(argv[i], "append_features", obj);
			call(argv[i], "included", obj);
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
	
	value_t nesting()
	{
		auto result = Array::allocate();
		auto scope = context->frame->prev->scope;

		for(size_t i = 0; i < scope->entries - 1; ++i)
			result->vector.push((*scope)[i]);

		return result;
	}
	
	value_t Module::alias_method(Module *self, Symbol *new_name, Symbol *old_name)
	{
		auto method = lookup_module_method(self, old_name);

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
	
	void Module::constants(Array *array, bool super)
	{
		if(vars)
			ValueMapAccess::each_pair(vars, [&](value_t key, value_t value) -> bool {
				if(cast<Symbol>(key)->is_constant())
					array->vector.push(key);

				return true;
			});

		if(super)
		{
			if(superclass)
				superclass->constants(array, true);
		}
	}
	
	value_t Module::rb_constants(Module *obj, value_t super)
	{
		auto result = Array::allocate();

		obj->constants(result, super ? super->test() : true);

		return result;
	}
	
	void Module::instance_methods(Array *array, bool super)
	{
		if(methods)
			ValueMapAccess::each_pair(methods, [&](value_t key, value_t value) -> bool {
				if(value != value_undef)
					array->vector.push(key);

				return true;
			});

		if(super)
		{
			if(superclass)
				superclass->instance_methods(array, true);
		}
	}
	
	value_t Module::rb_instance_methods(Module *obj, value_t super)
	{
		auto result = Array::allocate();

		obj->instance_methods(result, super ? super->test() : true);

		return result;
	}
	
	value_t Module::module_function(Module *obj, size_t argc, value_t argv[])
	{
		Class *meta = obj->instance_of;

		for(size_t i = 0; i < argc; ++i)
		{
			auto symbol = raise_cast<Symbol>(argv[i]);

			auto method = lookup_module_method(obj, symbol); 
			
			meta->set_method(symbol, method);
		}

		return obj;
	}
	
	value_t Module::method_defined(Module *obj, Symbol *name)
	{
		return Value::from_bool(lookup_module(obj, name) != 0);
	}

	value_t Module::define_method(Module *obj, Symbol *name, Proc *proc, value_t block)
	{
		if(!proc)
			proc = get_proc(block);

		auto method = new (collector) Method(proc->block, proc->scope, proc->scopes);

		obj->set_method(name, method);

		return proc;
	}
	
	value_t Module::undef_method(Module *obj, Symbol *name)
	{
		if(obj->get_method(name) == value_undef)
			return obj;

		lookup_module_method(obj, name);

		obj->set_method(name, value_undef);

		return obj;
	}
	
	value_t Module::module_eval(Module *obj, String *string, value_t block)
	{
		if(!string)
		{
			Proc *proc = get_proc(block);

			return call_code(proc->block, obj, proc->name, context->frame->scope->copy_and_prepend(obj), proc->scopes, block, 0, nullptr);
		}
		else
		{
			CharArray code = string->string.c_str();

			return eval(obj, Symbol::get("in eval"), context->frame->scope->copy_and_prepend(obj), code.str_ref(), code.str_length(), "(eval)");
		}
	}
	
	void Module::initialize()
	{
		method<Self<Module>, Optional<String>, Arg::Block, &module_eval>(context->object_class, "module_eval");
		method<Self<Module>, Optional<String>, Arg::Block, &module_eval>(context->object_class, "class_eval");
		method<Self<Value>, &to_s>(context->module_class, "to_s");
		method<Self<Module>, Symbol, Symbol, &alias_method>(context->module_class, "alias_method");
		method<Self<Module>, Symbol, &undef_method>(context->module_class, "undef_method");
		method<Self<Module>, Symbol, Optional<Proc>, Arg::Block, &define_method>(context->module_class, "define_method");
		method<Self<Module>, Module, &append_features>(context->module_class, "append_features");
		method<Self<Value>, Arg::Count, Arg::Values, &include>(context->module_class, "include");
		method<Value, &included>(context->module_class, "included");
		method<Value, &included>(context->module_class, "extended");
		method<Self<Module>, Value, &extend_object>(context->module_class, "extend_object");
		method<Self<Module>, Arg::Count, Arg::Values, &attr_reader>(context->module_class, "attr");
		method<Self<Module>, Arg::Count, Arg::Values, &attr_reader>(context->module_class, "attr_reader");
		method<Self<Module>, Arg::Count, Arg::Values, &attr_writer>(context->module_class, "attr_writer");
		method<Self<Module>, Arg::Count, Arg::Values, &attr_accessor>(context->module_class, "attr_accessor");
		singleton_method<&nesting>(context->module_class, "nesting");
		
		method<Self<Module>, Optional<Value>, &rb_constants>(context->module_class, "constants");
		method<Self<Module>, Optional<Value>, &rb_instance_methods>(context->module_class, "instance_methods");
		method<Self<Module>, Optional<Value>, &rb_instance_methods>(context->module_class, "public_instance_methods");
		method<Self<Module>, Optional<Value>, &rb_instance_methods>(context->module_class, "protected_instance_methods");
		method<Self<Module>, Optional<Value>, &rb_instance_methods>(context->module_class, "private_instance_methods");
		
		method<Self<Module>, Arg::Count, Arg::Values, &module_function>(context->module_class, "module_function");
		
		method<Self<Value>, Arg::Count, Arg::Values, &Mirb::visibility_dummy>(context->module_class, "public");
		method<Self<Value>, Arg::Count, Arg::Values, &Mirb::visibility_dummy>(context->module_class, "private");
		method<Self<Value>, Arg::Count, Arg::Values, &Mirb::visibility_dummy>(context->module_class, "protected");
		method<Self<Value>, Arg::Count, Arg::Values, &Mirb::visibility_dummy>(context->module_class, "public_class_method");
		method<Self<Value>, Arg::Count, Arg::Values, &Mirb::visibility_dummy>(context->module_class, "private_class_method");
		
		method<Self<Module>, Symbol, &const_defined>(context->module_class, "const_defined?");
		method<Self<Module>, Symbol, &const_get>(context->module_class, "const_get");
		method<Self<Module>, Symbol, Value, &const_set>(context->module_class, "const_set");

		method<Self<Module>, Symbol, &method_defined>(context->module_class, "method_defined?");
	}
};

