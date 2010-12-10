#pragma once
#include "value.hpp"
#include "block.hpp"
#include "method.hpp"
#include "classes/symbol.hpp"

namespace Mirb
{
	#define MIRB_ARG_INDEX(index) (argc - 1 - (index))
	#define MIRB_ARG(index) (argv[MIRB_ARG_INDEX(index)])
	#define MIRB_ARG_EACH_RAW(i) for(size_t i = 0; i < argc; i++)
	#define MIRB_ARG_EACH(i) for(size_t i = argc - 1; i != (size_t)-1; i--)
	#define MIRB_ARG_EACH_REV(i) for(size_t i = 0; i < argc; i++)

	class CharArray;
	class Exception;
	
	extern Exception *current_exception;

	value_t class_of(value_t obj);
	value_t real_class(value_t obj);
	value_t real_class_of(value_t obj);

	value_t singleton_class(value_t object);

	value_t define_class(value_t under, Symbol *name, value_t super);
	value_t define_class(value_t under, std::string name, value_t super);
	value_t define_module(value_t under, Symbol *name);
	value_t define_module(value_t under, std::string name);

	void include_module(value_t obj, value_t module);

	void class_name(value_t obj, value_t under, Symbol *name);
	value_t class_create_unnamed(value_t super);
	value_t class_create_bare(value_t super);
	value_t class_create_singleton(value_t object, value_t super);
	
	std::string inspect_object(value_t obj);
	CharArray inspect_obj(value_t obj);
	value_t inspect(value_t obj);

	CharArray pretty_inspect(value_t obj);
	
	ValueMap *get_vars(value_t obj);
	value_t get_const(value_t obj, Symbol *name, bool require = true);
	void set_const(value_t obj, Symbol *name, value_t value);
	value_t get_var(value_t obj, Symbol *name);
	void set_var(value_t obj, Symbol *name, value_t value);
	value_t get_ivar(value_t obj, Symbol *name);
	void set_ivar(value_t obj, Symbol *name, value_t value);
	
	Block *get_method(value_t obj, Symbol *name);
	void set_method(value_t obj, Symbol *name, Block *method);

	void initialize();
	void finalize();
	
	value_t raise(value_t exception_class, const CharArray &message);
	value_t raise(value_t exception);
	
	value_t eval(value_t self, Symbol *method_name, value_t method_module, const char_t *input, size_t length, CharArray &filename);
	
	Block *lookup_method(value_t module, Symbol *name, value_t *result_module);
	compiled_block_t lookup(value_t obj, Symbol *name, value_t *result_module);
	compiled_block_t lookup_super(value_t module, Symbol *name, value_t *result_module);

	compiled_block_t lookup_nothrow(value_t obj, Symbol *name, value_t *result_module);
	compiled_block_t lookup_super_nothrow(value_t module, Symbol *name, value_t *result_module);

	value_t call_code(compiled_block_t code, value_t obj, Symbol *name, value_t module, value_t block, size_t argc, value_t argv[]);
	
	value_t call(value_t obj, Symbol *name, value_t block, size_t argc, value_t argv[]);
	
	template<size_t length> value_t call(value_t obj, const char (&name)[length], value_t block, size_t argc, value_t argv[])
	{
		return call(obj, symbol_cast(name), block, argc, argv);
	}
	
	template<typename T> value_t call(value_t obj, T&& name, size_t argc, value_t argv[])
	{
		return call(obj, symbol_cast(name), value_nil, argc, argv);
	}
	
	template<typename T> value_t call(value_t obj, T&& name)
	{
		return call(obj, symbol_cast(name), value_nil, 0, 0);
	}

	value_t yield(value_t obj, value_t block, size_t argc, value_t argv[]);
	value_t yield(value_t obj, size_t argc, value_t argv[]);
	value_t yield(value_t obj);

	CharArray backtrace();
	
	String *enforce_string(value_t obj);
	
	void setup_classes();
	
	extern value_t main;
};

