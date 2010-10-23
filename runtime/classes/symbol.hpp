#pragma once
#include "../classes.hpp"
#include "string.hpp"

struct rt_symbol {
	struct rt_common common;
};

#define RT_SYMBOL(value) ((struct rt_symbol *)(value))

extern rt_value rt_Symbol;

void rt_symbol_setup(rt_value symbol);

rt_value rt_symbol_from_cstr(const char *name);

const char *rt_symbol_to_cstr(rt_value value);

static inline rt_value rt_symbol_from_string(rt_value value)
{
	return rt_string_from_cstr(rt_symbol_to_cstr(value));
}

static inline rt_value rt_string_from_symbol(rt_value value)
{
	return rt_string_from_cstr(rt_symbol_to_cstr(value));
}

void rt_symbol_init(void);

rt_compiled_block(rt_symbol_to_s);
