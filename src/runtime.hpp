#pragma once
#include "value.hpp"
#include "block.hpp"
#include "method.hpp"
#include "on-stack.hpp"
#include "classes/symbol.hpp"

namespace Mirb
{
	class CharArray;
	class Exception;
	
	extern Exception *current_exception;
	extern Frame *current_exception_frame_origin;

	void set_current_exception(Exception *exception);

	value_t class_of(value_t obj) prelude_nonnull(1);
	value_t real_class(value_t obj) prelude_nonnull(1);
	value_t real_class_of(value_t obj);

	Class *singleton_class(value_t object);
	
	Class *define_class(Module *under, Symbol *name, Class *super);
	Class *define_class(Module *under, std::string name, Class *super);
	Module *define_module(Module *under, Symbol *name);
	Module *define_module(Module *under, std::string name);

	/*
	 * include_module (calls Ruby code)
	 */
	void include_module(Module *obj, Module *module);
	
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

	value_t test_const(value_t obj, Symbol *name);
	value_t get_const(value_t obj, Symbol *name);
	value_t set_const(value_t obj, Symbol *name, value_t value);

	value_t get_var(value_t obj, Symbol *name);
	void set_var(value_t obj, Symbol *name, value_t value);
	value_t get_ivar(value_t obj, Symbol *name);
	void set_ivar(value_t obj, Symbol *name, value_t value);
	
	Block *get_method(value_t obj, Symbol *name);
	void set_method(value_t obj, Symbol *name, Block *method);

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
	value_t eval(value_t self, Symbol *method_name, value_t method_module, const char_t *input, size_t length, const CharArray &filename, bool free_input = false) prelude_use_result;
	
	Block *lookup_method(value_t module, Symbol *name, value_t *result_module);

	/*
	 * lookup
	 */
	Block *lookup(value_t obj, Symbol *name, value_t *result_module);

	/*
	 * lookup_super
	 */
	Block *lookup_super(value_t module, Symbol *name, value_t *result_module);

	Block *lookup_nothrow(value_t obj, Symbol *name, value_t *result_module);
	Block *lookup_super_nothrow(value_t module, Symbol *name, value_t *result_module);
	
	/*
	 * call_code (calls Ruby code)
	 */
	value_t call_code(Block *code, value_t obj, Symbol *name, value_t module, value_t block, size_t argc, value_t argv[]) prelude_use_result;
	value_t call_frame(Frame &frame) prelude_use_result;
	
	/*
	 * call (calls Ruby code, argv does not need to be marked)
	 */
	value_t call(value_t obj, Symbol *name, value_t block, size_t argc, value_t argv[]) prelude_use_result;
	
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

