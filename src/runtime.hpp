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

	value_t class_of(value_t obj);
	value_t real_class(value_t obj);
	value_t real_class_of(value_t obj);

	value_t singleton_class(value_t object);
	
	value_t define_class(value_t under, Symbol *name, value_t super);
	value_t define_class(value_t under, std::string name, value_t super);
	value_t define_module(value_t under, Symbol *name);
	value_t define_module(value_t under, std::string name);

	/*
	 * include_module (calls Ruby code)
	 */
	void include_module(value_t obj, value_t module);
	
	void class_name(value_t obj, value_t under, Symbol *name);
	value_t class_create_unnamed(value_t super);
	value_t class_create_bare(value_t super);
	value_t class_create_singleton(value_t object, value_t super);
	
	/*
	 * inspect_object (calls Ruby code)
	 */
	std::string inspect_object(value_t obj);

	/*
	 * inspect_obj (calls Ruby code)
	 */
	CharArray inspect_obj(value_t obj);

	/*
	 * inspect (calls Ruby code)
	 */
	value_t inspect(value_t obj);
	
	/*
	 * pretty_inspect (calls Ruby code)
	 */
	CharArray pretty_inspect(value_t obj);
	
	ValueMap *get_vars(value_t obj);

	value_t get_const(value_t obj, Symbol *name);
	bool set_const(value_t obj, Symbol *name, value_t value);

	value_t get_var(value_t obj, Symbol *name);
	void set_var(value_t obj, Symbol *name, value_t value);
	value_t get_ivar(value_t obj, Symbol *name);
	void set_ivar(value_t obj, Symbol *name, value_t value);
	
	Block *get_method(value_t obj, Symbol *name);
	void set_method(value_t obj, Symbol *name, Block *method);

	void initialize();
	void finalize();
	
	/*
	 * raise (calls Ruby code)
	 */
	bool type_error(value_t value, value_t expected);
	value_t raise(value_t exception_class, const CharArray &message);

	value_t raise(value_t exception);
	
	/*
	 * eval (calls Ruby code)
	 */
	value_t eval(value_t self, Symbol *method_name, value_t method_module, const char_t *input, size_t length, CharArray &filename, bool free_input = false);
	
	Block *lookup_method(value_t module, Symbol *name, value_t *result_module);

	/*
	 * lookup (calls Ruby code)
	 */
	Block *lookup(value_t obj, Symbol *name, value_t *result_module);

	/*
	 * lookup_super (calls Ruby code)
	 */
	Block *lookup_super(value_t module, Symbol *name, value_t *result_module);

	Block *lookup_nothrow(value_t obj, Symbol *name, value_t *result_module);
	Block *lookup_super_nothrow(value_t module, Symbol *name, value_t *result_module);
	
	/*
	 * call_code (calls Ruby code)
	 */
	value_t call_code(Block *code, value_t obj, Symbol *name, value_t module, value_t block, size_t argc, value_t argv[]);
	value_t call_frame(Frame &frame);
	
	/*
	 * call (calls Ruby code)
	 */
	value_t call(value_t obj, Symbol *name, value_t block, size_t argc, value_t argv[]);
	
	/*
	 * call (calls Ruby code)
	 */
	template<size_t length> value_t call(value_t obj, const char (&name)[length], value_t block, size_t argc, value_t argv[])
	{
		return call(obj, symbol_cast(name), block, argc, argv);
	}
	
	/*
	 * call (calls Ruby code)
	 */
	template<typename T> value_t call(value_t obj, T&& name, size_t argc, value_t argv[])
	{
		return call(obj, symbol_cast(name), value_nil, argc, argv);
	}
	
	/*
	 * call (calls Ruby code)
	 */
	template<typename T> value_t call(value_t obj, T&& name)
	{
		return call(obj, symbol_cast(name), value_nil, 0, 0);
	}
	
	/*
	 * yield (calls Ruby code)
	 */
	value_t yield(value_t obj, value_t block, size_t argc, value_t argv[]);

	/*
	 * yield (calls Ruby code)
	 */
	value_t yield(value_t obj, size_t argc, value_t argv[]);

	/*
	 * yield (calls Ruby code)
	 */
	value_t yield(value_t obj);
	
	/*
	 * backtrace (calls Ruby code)
	 */
	CharArray backtrace();
	
	/*
	 * enforce_string (calls Ruby code)
	 */
	String *enforce_string(value_t obj);
	
	void setup_classes();
	
	extern value_t main;
};

