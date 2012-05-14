#pragma once
#include "value.hpp"
#include "block.hpp"
#include "on-stack.hpp"
#include "classes/symbol.hpp"

namespace Mirb
{
	class CharArray;
	class Exception;
	
	void set_current_exception(Exception *exception);

	Class *class_of(value_t obj) prelude_nonnull(1);
	bool is_real_class(Class *obj);
	Class *real_class(Class *obj) prelude_nonnull(1);
	Class *real_class_of(value_t obj);

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
	 * inspect_object (calls Ruby code)
	 */
	std::string inspect_object(value_t obj);

	/*
	 * inspect_obj (calls Ruby code)
	 */
	CharArray inspect_obj(value_t obj);
	bool append_inspect(CharArray &result, value_t obj);

	/*
	 * inspect (calls Ruby code)
	 */
	value_t inspect(value_t obj);
	
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
	value_t set_const(value_t obj, Symbol *name, value_t value);
	
	value_t get_var_raw(value_t obj, Symbol *name);
	value_t get_var(value_t obj, Symbol *name);
	void set_var(value_t obj, Symbol *name, value_t value);
	
	value_t get_global(Symbol *name);
	void set_global(Symbol *name, value_t value);
	
	void initialize();
	void finalize();
	
	/*
	 * raise
	 */
	bool type_error(value_t value, value_t expected);
	value_t raise(Class *exception_class, const CharArray &message);

	value_t raise(value_t exception);

	void swallow_exception();

	bool validate_return(value_t &result);
	
	/*
	 * eval (calls Ruby code)
	 */
	value_t eval(value_t self, Symbol *method_name, Tuple<Module> *scope, const char_t *input, size_t length, const CharArray &filename, bool free_input = false) prelude_use_result;
	
	Method *lookup_method(Module *module, Symbol *name);

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
	value_t call_code(Block *code, value_t obj, Symbol *name, Tuple<Module> *scope, value_t block, size_t argc, value_t argv[]) prelude_use_result;
	value_t call_frame(Frame &frame) prelude_use_result;
	
	/*
	 * call (calls Ruby code, argv does not need to be marked)
	 */
	value_t call(Block *code, value_t obj, Symbol *name, Tuple<Module> *scope, value_t block, size_t argc, value_t argv[]) prelude_use_result;
	value_t call(value_t obj, Symbol *name, value_t block, size_t argc, value_t argv[]) prelude_use_result;
	value_t call(value_t obj, Symbol *name) prelude_use_result;
	
	/*
	 * call (calls Ruby code, argv does not need to be marked)
	 */
	template<size_t length> value_t call(value_t obj, const char (&name)[length], value_t block, size_t argc, value_t argv[]) prelude_use_result;
	template<size_t length> value_t call(value_t obj, const char (&name)[length], value_t block, size_t argc, value_t argv[])
	{
		return call(obj, symbol_cast(name), block, argc, argv);
	}
	
	/*
	 * call (calls Ruby code, argv does not need to be marked)
	 */
	template<typename T> value_t call(value_t obj, T&& name, size_t argc, value_t argv[]) prelude_use_result;
	template<typename T> value_t call(value_t obj, T&& name, size_t argc, value_t argv[])
	{
		return call(obj, symbol_cast(name), value_nil, argc, argv);
	}
	
	/*
	 * call (calls Ruby code, argv does not need to be marked)
	 */
	template<typename T> value_t call(value_t obj, T&& name) prelude_use_result;
	template<typename T> value_t call(value_t obj, T&& name)
	{
		return call(obj, symbol_cast(name), value_nil, 0, 0);
	}
	
	/*
	 * yield (calls Ruby code, argv does not need to be marked)
	 */
	value_t yield(value_t obj, value_t block, size_t argc, value_t argv[]) prelude_use_result;

	/*
	 * yield (calls Ruby code, argv does not need to be marked)
	 */
	value_t yield(value_t obj, size_t argc, value_t argv[]) prelude_use_result;

	/*
	 * yield (calls Ruby code, argv does not need to be marked)
	 */
	value_t yield(value_t obj) prelude_use_result;
	
	/*
	 * backtrace
	 */
	Tuple<StackFrame> *backtrace();
	
	/*
	 * enforce_string (calls Ruby code)
	 */
	String *enforce_string(value_t obj);
	
	void setup_classes();
};

#include "method.hpp"
