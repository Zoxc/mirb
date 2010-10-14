#include "../../src/symbol-pool.hpp"

extern "C"
{

#include "../../globals.h"
#include "symbol.h"
#include "string.h"

rt_value rt_Symbol;

const char *rt_symbol_to_cstr(rt_value value)
{
	return (const char *)((Mirb::Symbol *)value)->string.c_str;
}

void rt_symbol_setup(rt_value symbol)
{
	RT_COMMON(symbol)->flags = C_SYMBOL;
	RT_COMMON(symbol)->class_of = rt_Symbol;
	RT_COMMON(symbol)->vars = 0;
}

rt_value rt_symbol_from_cstr(const char* name)
{
	return (rt_value)Mirb::symbol_pool.get(name);
}


/*
 * Symbol
 */

rt_compiled_block(rt_symbol_inspect)
{
	rt_value result = rt_string_from_cstr(":");
	rt_concat_string(result, rt_string_from_cstr(rt_symbol_to_cstr(obj)));

	return result;
}

rt_compiled_block(rt_symbol_to_s)
{
	return rt_string_from_cstr(rt_symbol_to_cstr(obj));
}

void rt_symbol_init(void)
{
	rt_define_method(rt_Symbol, "to_s", rt_symbol_to_s);
	rt_define_method(rt_Symbol, "inspect", rt_symbol_inspect);
}

};
