#pragma once
#include "value.hpp"
#include "block.hpp"
#include "method.hpp"

namespace Mirb
{
	#define MIRB_ARG_INDEX(index) (argc - 1 - (index))
	#define MIRB_ARG(index) (argv[MIRB_ARG_INDEX(index)])
	#define MIRB_ARG_EACH_RAW(i) for(size_t i = 0; i < argc; i++)
	#define MIRB_ARG_EACH(i) for(size_t i = argc - 1; i != (size_t)-1; i--)
	#define MIRB_ARG_EACH_REV(i) for(size_t i = 0; i < argc; i++)

	class Symbol;
	class CharArray;
	
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
	value_t inspect(value_t obj);
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

	value_t eval(value_t self, Symbol *method_name, value_t method_module, const char_t *input, size_t length, CharArray &filename);
	
	Block *lookup_method(value_t module, Symbol *name, value_t *result_module);
	compiled_block_t lookup(value_t obj, Symbol *name, value_t *result_module);
	compiled_block_t lookup_super(value_t module, Symbol *name, value_t *result_module);

	value_t call_code(compiled_block_t code, value_t obj, Symbol *name, value_t module, value_t block, size_t argc, value_t argv[]);
	
	// TODO: Replace const char * with literal templates
	value_t call(value_t obj, Symbol *name, value_t block, size_t argc, value_t argv[]);
	value_t call(value_t obj, const char *name, size_t argc, value_t argv[]);
	value_t call(value_t obj, const char *name, value_t block, size_t argc, value_t argv[]);
	value_t call(value_t obj, Symbol *name, size_t argc, value_t argv[]);
	value_t call(value_t obj, Symbol *name);
	value_t call(value_t obj, const char *name);

	CharArray backtrace();
	
	void setup_classes();

	extern value_t main;
};

