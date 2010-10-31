#pragma once
#include "../classes.hpp"

struct rt_string {
	struct rt_common common;
	char* string;
	size_t length;
};

#undef RT_STRING
#define RT_STRING(value) ((struct rt_string *)(value))

extern rt_value rt_String;

rt_value rt_string_from_raw_str(char *str, size_t length);
rt_value rt_string_from_cstr(const char *str);
rt_value rt_string_from_str(const char *str, size_t length);
rt_value rt_string_from_hex(rt_value value);
rt_value rt_string_from_int(rt_value value);

static inline const char *rt_string_to_cstr(rt_value value)
{
	return RT_STRING(value)->string;
}

static inline rt_value rt_dup_string(rt_value value)
{
	return rt_string_from_str(RT_STRING(value)->string, RT_STRING(value)->length);
}

rt_compiled_block(rt_string_concat);

static inline void rt_concat_string(rt_value string, rt_value add)
{
	rt_string_concat(RT_NIL, RT_NIL, string, 0, 1, &add);
}

void rt_string_init(void);


