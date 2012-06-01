#pragma once
#include "value.hpp"
#include "block.hpp"
#include "on-stack.hpp"
#include "finally.hpp"
#include "classes/symbol.hpp"

namespace Mirb
{
	class CharArray;
	class Exception;

	struct InternalException
	{
		InternalException(Exception *value) : value(value)
		{
			Value::assert_valid((value_t)value);
		}

		Exception *value;
	};

	value_t coerce(value_t left, Symbol *name, value_t right);

	void assert_stack_space();

	bool map_index(intptr_t index, size_t size, size_t &result);
	
	bool subclass_of(Class *super, Class *c);
	bool kind_of(Class *klass, value_t obj);
	Class *internal_class_of(value_t obj) prelude_nonnull(1);
	bool is_real_class(Class *obj);
	Class *real_class(Class *obj) prelude_nonnull(1);
	Class *class_of(value_t obj);

	Class *singleton_class(value_t object);
	
	Class *define_class(value_t under, Symbol *name, Class *super);
	Module *define_module(value_t under, Symbol *name);
	
	Class *define_scoped_class(value_t obj, Symbol *name, Class *super);
	Module *define_scoped_module(value_t obj, Symbol *name);
	
	Class *define_class(const CharArray &name, Class *super);
	Module *define_module(const CharArray &name);

	/*
	 * include_module (calls Ruby code)
	 */
	void include_module(Module *obj, Module *module) prelude_nonnull(1, 2);
	
	void class_name(value_t obj, Module *under, Symbol *name);
	Class *class_create_unnamed(Class *super);
	Class *class_create_bare(Class *super);
	Class *class_create_singleton(value_t object, Class *super);
	
	/*
	 * inspect_obj (calls Ruby code)
	 */
	CharArray inspect(value_t obj);

	/*
	 * inspect (calls Ruby code)
	 */
	String *inspect_obj(value_t obj);
	
	/*
	 * pretty_inspect (calls Ruby code)
	 */
	CharArray pretty_inspect(value_t obj);
	
	ValueMap *get_vars(value_t obj);
	
	/*
	 * scope_path (calls Ruby code)
	 */
	CharArray scope_path(Tuple<Module> *scope);

	value_t test_const(Tuple<Module> *scope, Symbol *name);
	value_t get_scoped_const(value_t obj, Symbol *name);
	value_t get_const(Tuple<Module> *scope, Symbol *name);
	void set_const(value_t obj, Symbol *name, value_t value);
	
	value_t get_var_raw(value_t obj, Symbol *name);
	value_t get_var(value_t obj, Symbol *name);
	void set_var(value_t obj, Symbol *name, value_t value);
	
	template<typename T> value_t get_var(value_t obj, T &&name)
	{
		return get_var(obj, symbol_cast(std::forward<T>(name)));
	}
	
	template<typename T> void set_var(value_t obj, T &&name, value_t value)
	{
		return set_var(obj, symbol_cast(std::forward<T>(name)), value);
	}

	Global *get_global_object(Symbol *name, bool force = false);
	void set_global_object(Symbol *name, Global *global);
	
	/*
	 * get_global, set_global (calls Ruby code)
	 */
	value_t get_global(Symbol *name);
	void set_global(Symbol *name, value_t value);
	
	template<typename T> value_t get_global(T &&name)
	{
		return get_global(symbol_cast(std::forward<T>(name)));
	}
	
	template<typename T> void set_global(T &&name, value_t value)
	{
		return set_global(symbol_cast(std::forward<T>(name)), value);
	}

	void initialize_thread();
	void initialize(bool console = false);
	void finalize();
	
	/*
	 * compare (calls Ruby code)
	 */
	int compare(value_t left, value_t right);

	template<typename F> void trap_exception(Exception *&exception, F func)
	{
		try
		{
			func();
			exception = 0;
		}
		catch(InternalException e)
		{
			exception = e.value;
		}
	}

	/*
	 * raise
	 */
	prelude_noreturn void type_error(value_t value, const CharArray &expected);
	prelude_noreturn void coerce_error(value_t left, value_t right);
	prelude_noreturn void method_error(Symbol *name, value_t obj, bool in);
	prelude_noreturn void zero_division_error();
	void type_error(value_t value, Class *expected);
	Exception *create_exception(Class *exception_class, const CharArray &message);
	prelude_noreturn void raise(Class *exception_class, const CharArray &message);

	bool stack_no_reserve(Frame &frame);
	
	template<typename T> T *raise_cast(value_t obj);

	template<typename T> T *raise_cast(value_t obj)
	{
		if(Value::of_type<T>(obj))
			return reinterpret_cast<T *>(obj);
		else
			type_error(obj, "an object of type " + Mirb::Value::names[Mirb::Value::TypeTag<T>::value]);
	}
	
	template<Value::Type type> value_t raise_cast(value_t obj)
	{
		if(Value::type(obj) != type)
			type_error(obj, "an object of type " + Mirb::Value::names[type]);

		return obj;
	}

	/*
	 * eval (calls Ruby code)
	 */
	value_t eval(value_t self, Symbol *method_name, Tuple<Module> *scope, const char_t *input, size_t length, const CharArray &filename, bool free_input = false);
	
	Method *lookup_module(Module *module, Symbol *name);
	Method *lookup_method(Module *module, Symbol *name, value_t obj);

	Method *respond_to(value_t obj, Symbol *name);
	
	template<typename T> Method *respond_to(value_t obj, T &&name)
	{
		return respond_to(obj, symbol_cast(std::forward<T>(name)));
	}

	/*
	 * lookup
	 */
	Method *lookup(value_t obj, Symbol *name);

	/*
	 * lookup_super
	 */
	Method *lookup_super(Module *module, Symbol *name);
	
	/*
	 * call_code (calls Ruby code)
	 */
	value_t call_code(Block *code, value_t obj, Symbol *name, Tuple<Module> *scope, Tuple<> *scopes, value_t block, size_t argc, value_t argv[]);
	value_t call_frame(Frame &frame);
	
	/*
	 * call_argv (calls Ruby code, argv does not need to be marked)
	 */
	value_t call_argv(Method *method, value_t obj, Symbol *name, value_t block, size_t argc, value_t argv[]);
	value_t call_argv(value_t obj, Symbol *name, value_t block, size_t argc, value_t argv[]);
	
	/*
	 * call_argv (calls Ruby code, argv does not need to be marked)
	 */
	template<typename T> value_t call_argv(value_t obj, T &&name, value_t block, size_t argc, value_t argv[]);
	template<typename T> value_t call_argv(value_t obj, T &&name, value_t block, size_t argc, value_t argv[])
	{
		return call_argv(obj, symbol_cast(std::forward<T>(name)), block, argc, argv);
	}
	
	/*
	 * call_argv (calls Ruby code, argv does not need to be marked)
	 */
	template<typename T> value_t call_argv(value_t obj, T&& name, size_t argc, value_t argv[]);
	template<typename T> value_t call_argv(value_t obj, T&& name, size_t argc, value_t argv[])
	{
		return call_argv(obj, symbol_cast(std::forward<T>(name)), value_nil, argc, argv);
	}
	
	/*
	 * call (calls Ruby code, argv does not need to be marked)
	 */
	template<typename T> value_t call(value_t obj, T&& name);
	template<typename T> value_t call(value_t obj, T&& name)
	{
		return call_argv(obj, symbol_cast(std::forward<T>(name)), value_nil, 0, 0);
	}
	
	Proc *get_proc(value_t obj);

	/*
	 * yield (calls Ruby code, argv does not need to be marked)
	 */
	value_t yield_argv(value_t obj, value_t block, size_t argc, value_t argv[]);

	/*
	 * yield (calls Ruby code, argv does not need to be marked)
	 */
	value_t yield_argv(value_t obj, size_t argc, value_t argv[]);

	/*
	 * yield (calls Ruby code, argv does not need to be marked)
	 */
	value_t yield(value_t obj);
	
	/*
	 * backtrace
	 */
	Tuple<StackFrame> *backtrace(Frame *from = 0);
	
	Array *cast_array(value_t value);
	String *cast_string(value_t value);
	value_t cast_integer(value_t value);

	void setup_classes();
};

#include "method.hpp"
